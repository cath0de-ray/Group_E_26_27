#include <Arduino.h>
#include "driver/i2s_std.h"
#include <VITALIS_Triage_inferencing.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
// ================= PINS & TIMEOUTS =================
#define I2S_BCLK 2
#define I2S_WS 21
#define I2S_DIN 16
#define BUTTON_PIN 1 
// LED_BUILTIN is natively defined. Active-LOW on XIAO ESP32C6.

const unsigned long INACTIVITY_TIMEOUT = 120000; // 2 minutes
const unsigned long HOLD_TO_SLEEP_TIME = 3000;   // 3 seconds
const unsigned long DEBOUNCE_DELAY = 50;         // 50ms physical bounce filter

// ================= GLOBALS =================
#define SAMPLE_RATE 16000
#define BUFFER_LEN 256
#define EI_INPUT_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

int32_t i2s_buffer[BUFFER_LEN];
float features[EI_INPUT_SIZE];
i2s_chan_handle_t rx_handle = NULL;

bool recording = false;
unsigned long recordStart = 0;
unsigned long lastActivity = 0;
unsigned long lastBlink = 0;
bool ledState = HIGH; 

// Bulletproof Button State Machine
bool lastFlickerState = HIGH;
bool validatedButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long buttonPressTime = 0;
bool buttonHeld = false;

BLECharacteristic *pCharacteristic;

// ================= FILTER & SMOOTH =================
float prev_input = 0, prev_output_hp = 0, prev_output_lp = 0, envelope = 0;
float alpha_hp = 0.95, alpha_lp = 0.1, env_alpha = 0.9;

float smoothed_scores[EI_CLASSIFIER_LABEL_COUNT] = {0};
float alpha = 0.7;

// ================= RESULT TRACK =================
float best_score = -1.0;
String best_label = "Uncertain";

// ================= SIGNAL =================
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

// ================= FILTER FUNCTION =================
float process_sample(float x) {
    static float dc = 0;
    dc = 0.999 * dc + 0.001 * x;
    x -= dc;
    float hp = alpha_hp * (prev_output_hp + x - prev_input);
    prev_input = x;
    prev_output_hp = hp;
    float lp = prev_output_lp + alpha_lp * (hp - prev_output_lp);
    prev_output_lp = lp;
    envelope = env_alpha * envelope + (1 - env_alpha) * abs(lp);
    return envelope;
}

// ================= SLEEP ROUTINE =================
// ================= SLEEP ROUTINE =================
void enterDeepSleep() {
    Serial.println("Initiating Sleep Sequence...");
    digitalWrite(LED_BUILTIN, HIGH); // LED OFF (Active-Low)
    
    // Graceful Hardware Shutdown
    if (pCharacteristic) {
        pCharacteristic->setValue("SLEEP");
        pCharacteristic->notify();
    }
    
    if (rx_handle != NULL) {
        i2s_channel_disable(rx_handle);
    }
    
    BLEDevice::deinit(true);
    Serial.flush();
    
    // HARDWARE LOCKOUT: Wait for physical release
    while (digitalRead(BUTTON_PIN) == LOW) {
        delay(10);
    }
    delay(200); // Wait for final mechanical release bounce to settle
    
    Serial.println("Powering down.");

    // ===== THE CRITICAL FIX =====
    // Hand over pull-up control to the RTC domain so the pin doesn't float LOW in sleep
    rtc_gpio_pullup_en((gpio_num_t)BUTTON_PIN);
    rtc_gpio_pulldown_dis((gpio_num_t)BUTTON_PIN);

    // Configure RTC domain to wake up on BUTTON_PIN LOW
    esp_sleep_enable_ext1_wakeup(1ULL << BUTTON_PIN, ESP_EXT1_WAKEUP_ANY_LOW); 
    esp_deep_sleep_start();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // LED ON when awake (Active-Low)

    lastActivity = millis();
    Serial.println("AI STETHOSCOPE READY - AWAKE");

    // ===== BLE =====
    BLEDevice::init("VITALIS");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService("FFE0");
    pCharacteristic = pService->createCharacteristic(
        "FFE1",
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pService->start();
    BLEDevice::getAdvertising()->start();

    // ===== I2S =====
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)I2S_BCLK,
            .ws = (gpio_num_t)I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = (gpio_num_t)I2S_DIN,
        },
    };
    i2s_channel_init_std_mode(rx_handle, &std_cfg);
    i2s_channel_enable(rx_handle);
}

void loop() {
    bool rawReading = digitalRead(BUTTON_PIN);

    // ===== DEBOUNCE LOGIC =====
    if (rawReading != lastFlickerState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
        // If the state has stabilized beyond the bounce window
        if (rawReading != validatedButtonState) {
            validatedButtonState = rawReading;

            if (validatedButtonState == LOW) {
                // FALLING EDGE (Clean Press)
                buttonPressTime = millis();
                buttonHeld = false;
                lastActivity = millis();
            } else {
                // RISING EDGE (Clean Release)
                if (!buttonHeld && !recording) {
                    recording = true;
                    recordStart = millis();
                    best_score = -1.0;
                    best_label = "Uncertain";
                    memset(smoothed_scores, 0, sizeof(smoothed_scores));
                    Serial.println("Recording Started");
                }
                lastActivity = millis();
            }
        }
    }
    lastFlickerState = rawReading;

    // ===== HOLD DETECT =====
    // Evaluated continuously if the debounced state is LOW
    if (validatedButtonState == LOW && !buttonHeld) {
        if (millis() - buttonPressTime >= HOLD_TO_SLEEP_TIME) {
            buttonHeld = true;
            enterDeepSleep();
        }
    }

    // ===== INACTIVITY TIMEOUT =====
    if (!recording && (millis() - lastActivity > INACTIVITY_TIMEOUT)) {
        Serial.println("Inactivity Timeout.");
        enterDeepSleep();
    }

    // ===== LED LOGIC =====
    if (recording) {
        if (millis() - lastBlink > 20) {
            lastBlink = millis();
            ledState = !ledState;
            digitalWrite(LED_BUILTIN, ledState);
        }
    } else {
        digitalWrite(LED_BUILTIN, LOW); // Solid ON when idle (Active-Low)
    }

    // ===== EXECUTE RECORDING (If Active) =====
    if (recording) {
        size_t bytes_read = 0;
        i2s_channel_read(rx_handle, i2s_buffer, sizeof(i2s_buffer), &bytes_read, 10);
        int samples = bytes_read / 4;

        // Sliding Window
        memmove(features, features + samples, (EI_INPUT_SIZE - samples) * sizeof(float));
        for (int i = 0; i < samples && i < EI_INPUT_SIZE; i++) {
            float filtered = process_sample((float)(i2s_buffer[i] >> 14));
            features[EI_INPUT_SIZE - samples + i] = filtered;
        }

        // Run AI
        signal_t signal;
        signal.total_length = EI_INPUT_SIZE;
        signal.get_data = &get_signal_data;
        ei_impulse_result_t result = {0};
        run_classifier(&signal, &result, false);

        // Smooth
        for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            smoothed_scores[i] = alpha * smoothed_scores[i] + (1 - alpha) * result.classification[i].value;
        }

        float max_val = 0;
        const char* label = "Uncertain";
        for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
            if (smoothed_scores[i] > max_val) {
                max_val = smoothed_scores[i];
                label = result.classification[i].label;
            }
        }
        if (max_val < 0.70) label = "Uncertain";

        // Track Best
        if (max_val >= best_score) {
            best_score = max_val;
            best_label = label;
        }

        // Stop after 8 seconds
        if (millis() - recordStart > 8000) {
            recording = false;
            Serial.println("Recording Finished");

            char code = 'U';
            if (best_label == "Crackling Lungs") code = 'C';
            else if (best_label == "Normal Lungs") code = 'N';
            else if (best_label == "Ronchi Lungs") code = 'R';
            else if (best_label == "Wheezing Lungs") code = 'W';

            String msg = "F:" + String(best_score,3) + "|" + String(code);
            
            pCharacteristic->setValue(msg.c_str());
            pCharacteristic->notify();
            Serial.print("Sent: ");
            Serial.println(msg);
            Serial.println("Length:");
            Serial.println(msg.length());
        }
    }
    
    delay(40); // Preserving your requested delay
}