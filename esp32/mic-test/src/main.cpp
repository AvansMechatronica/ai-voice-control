#include <Arduino.h>
#include <driver/i2s.h>
#include "config.h"



#define SAMPLE_RATE 16000

void setup() {
  Serial.begin(115200);
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void loop() {
  int32_t sample;
  size_t bytes_read;

  i2s_read(I2S_PORT, &sample, sizeof(sample), &bytes_read, portMAX_DELAY);

  // schaal terug naar 16-bit (optioneel)
  int16_t audio = sample >> 14;

  Serial.println(audio);
}