#include <Arduino.h>
#include "driver/i2s_std.h"
#include <VITALIS_Triage_inferencing.h>

// ===== MIC PINS =====
#define I2S_BCLK 2
#define I2S_WS   21
#define I2S_DIN  16

#define SAMPLE_RATE 16000
#define BUFFER_LEN 256
#define EI_INPUT_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

int32_t i2s_buffer[BUFFER_LEN];
float features[EI_INPUT_SIZE];

i2s_chan_handle_t rx_handle = NULL;

// ===== FILTER STATE =====
float prev_input = 0;
float prev_output_hp = 0;
float prev_output_lp = 0;
float envelope = 0;

// tuning
float alpha_hp = 0.95;
float alpha_lp = 0.1;
float env_alpha = 0.9;

// ===== SMOOTHING =====
float smoothed_scores[EI_CLASSIFIER_LABEL_COUNT] = {0};
float alpha = 0.7;

// ===== SIGNAL =====
static int get_signal_data(size_t offset, size_t length, float *out_ptr) {
  memcpy(out_ptr, features + offset, length * sizeof(float));
  return 0;
}

// ===== FILTER FUNCTION =====
float process_sample(float x) {

  // DC removal
  static float dc = 0;
  dc = 0.999 * dc + 0.001 * x;
  x = x - dc;

  // High-pass
  float hp = alpha_hp * (prev_output_hp + x - prev_input);
  prev_input = x;
  prev_output_hp = hp;

  // Low-pass
  float lp = prev_output_lp + alpha_lp * (hp - prev_output_lp);
  prev_output_lp = lp;

  // Envelope
  envelope = env_alpha * envelope + (1 - env_alpha) * abs(lp);

  return envelope;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("FINAL AI SYSTEM START");

  // ===== I2S INIT =====
  i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  i2s_new_channel(&chan_cfg, NULL, &rx_handle);

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
          I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,
          .bclk = (gpio_num_t)I2S_BCLK,
          .ws   = (gpio_num_t)I2S_WS,
          .dout = I2S_GPIO_UNUSED,
          .din  = (gpio_num_t)I2S_DIN,
      },
  };

  i2s_channel_init_std_mode(rx_handle, &std_cfg);
  i2s_channel_enable(rx_handle);
}

void loop() {

  size_t bytes_read = 0;

  i2s_channel_read(rx_handle, i2s_buffer, sizeof(i2s_buffer),
                   &bytes_read, 10);

  int samples = bytes_read / 4;

  // ===== SLIDING WINDOW =====
  memmove(features, features + samples,
          (EI_INPUT_SIZE - samples) * sizeof(float));

  for (int i = 0; i < samples && i < EI_INPUT_SIZE; i++) {
    int32_t raw = i2s_buffer[i] >> 14;
    float filtered = process_sample((float)raw);

    features[EI_INPUT_SIZE - samples + i] = filtered;
  }

  // ===== RUN AI =====
  signal_t signal;
  signal.total_length = EI_INPUT_SIZE;
  signal.get_data = &get_signal_data;

  ei_impulse_result_t result = {0};
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);

  if (res != EI_IMPULSE_OK) {
    Serial.println("Inference error");
    return;
  }

  // ===== SMOOTHING =====
  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    smoothed_scores[i] =
        alpha * smoothed_scores[i] +
        (1 - alpha) * result.classification[i].value;
  }

  // ===== FIND BEST =====
  float max_val = 0;
  const char* label = "Uncertain";

  for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (smoothed_scores[i] > max_val) {
      max_val = smoothed_scores[i];
      label = result.classification[i].label;
    }
  }

  // ===== THRESHOLD =====
  if (max_val < 0.6) {
    label = "Uncertain";
  }

  // ===== OUTPUT =====
  Serial.print("Prediction: ");
  Serial.print(label);
  Serial.print(" | Confidence: ");
  Serial.println(max_val, 3);

  delay(50);
}