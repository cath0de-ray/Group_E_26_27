#include <Arduino.h>

#include "driver/i2s_std.h"

#include <VITALIS_Triage_inferencing.h>

#include <BLEDevice.h>

#include <BLEServer.h>

#include <BLEUtils.h>

#include <BLE2902.h>

// ================= MIC =================

#define I2S_BCLK 2

#define I2S_WS 21

#define I2S_DIN 16

#define SAMPLE_RATE 16000

#define BUFFER_LEN 256

#define EI_INPUT_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

int32_t i2s_buffer[BUFFER_LEN];

float features[EI_INPUT_SIZE];

i2s_chan_handle_t rx_handle = NULL;

// ================= BUTTON =================

#define BUTTON_PIN 1

bool recording = false;

unsigned long recordStart = 0;

// ================= BLE =================

BLECharacteristic *pCharacteristic;

// ================= FILTER =================

float prev_input = 0, prev_output_hp = 0, prev_output_lp = 0, envelope = 0;

float alpha_hp = 0.95, alpha_lp = 0.1, env_alpha = 0.9;

// ================= SMOOTH =================

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

void setup() {

Serial.begin(115200);

delay(1000);

pinMode(BUTTON_PIN, INPUT_PULLUP);

Serial.println("AI STETHOSCOPE READY");

// ===== BLE =====

// ===== BLE =====
BLEDevice::init("VITALIS");
BLEDevice::setMTU(247); // ADD THIS LINE. Requests max standard MTU.
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

i2s_chan_config_t chan_cfg =

I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);

i2s_new_channel(&chan_cfg, NULL, &rx_handle);

i2s_std_config_t std_cfg = {

.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),

.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(

I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),

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

// ===== BUTTON PRESS START =====

if (digitalRead(BUTTON_PIN) == LOW && !recording) {

recording = true;

recordStart = millis();

best_score = -1.0;

best_label = "Uncertain";

memset(smoothed_scores, 0, sizeof(smoothed_scores));

Serial.println("Recording Started");

delay(300); // debounce

}

if (!recording) return;

// ===== AUDIO READ =====

size_t bytes_read = 0;

i2s_channel_read(rx_handle, i2s_buffer, sizeof(i2s_buffer),

&bytes_read, 10);

int samples = bytes_read / 4;

// ===== SLIDING WINDOW =====

memmove(features, features + samples,

(EI_INPUT_SIZE - samples) * sizeof(float));

for (int i = 0; i < samples && i < EI_INPUT_SIZE; i++) {

float filtered = process_sample((float)(i2s_buffer[i] >> 14));

features[EI_INPUT_SIZE - samples + i] = filtered;

}

// ===== RUN AI =====

signal_t signal;

signal.total_length = EI_INPUT_SIZE;

signal.get_data = &get_signal_data;

ei_impulse_result_t result = {0};

run_classifier(&signal, &result, false);

// ===== SMOOTH =====

for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {

smoothed_scores[i] =

alpha * smoothed_scores[i] +

(1 - alpha) * result.classification[i].value;

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

// ===== TRACK BEST =====

if (max_val >= best_score) {

best_score = max_val;

best_label = label;

}

Serial.print("Scores -> ");

for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {

Serial.print(result.classification[i].label);

Serial.print(":");

Serial.print(smoothed_scores[i], 3);

Serial.print(" ");

}

Serial.print(" | Winner: ");

Serial.print(label);

Serial.print(" ");

Serial.println(max_val, 3);

// ===== STOP AFTER 8 SEC =====

if (millis() - recordStart > 8000) {

recording = false;

Serial.println("Recording Finished");

char code = 'U';

if (best_label == "Crackling Lungs") code = 'C';
else if (best_label == "Normal Lungs") code = 'N';
else if (best_label == "Ronchi Lungs") code = 'R';
else if (best_label == "Wheezing Lungs") code = 'W';

String msg = "F:" + String(best_score,3) + "|" + String(code);

Serial.println(msg); // DEBUG

pCharacteristic->setValue(msg.c_str());

pCharacteristic->notify();

Serial.print("Sent: ");

Serial.println(msg);
Serial.println("Length:");
Serial.println(msg.length());

}

delay(40);

}