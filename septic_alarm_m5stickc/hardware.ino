// This file is part of septic_alarm.
//
// septic_alarm is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the 
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// septic_alarm is distributed in the hope that it will be useful, 
// but WITHOUT ANY WARRANTY; without even the implied warranty of 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along 
// with septic_alarm. If not, see <https://www.gnu.org/licenses/>.

#include <M5StickCPlus.h>
#include <utility/ST7735_Defines.h>
#include <driver/i2s.h>
#include <time.h>

#include "event.h"

#define PIN_CLK 0
#define PIN_DATA 34
#define READ_LEN (2 * 256)
#define GAIN_FACTOR 3
#define NUM_SAMPLES 160

int alarm_sound_streak = 0;
static uint8_t raw_buffer[READ_LEN] = { 0 };
static int16_t *adcBuffer = NULL;


void checkSoundVolumeTask(void *arg) {
  size_t bytesread;

  while (1) {
    i2s_read(I2S_NUM_0, (char *)raw_buffer, READ_LEN, &bytesread,
             (100 / portTICK_RATE_MS));
    adcBuffer = (int16_t *)raw_buffer;

    last_sound_level = calculateSoundLevel();
    Serial.printf("Sound level: %.1f dB  (streak: %d)\n", last_sound_level, alarm_sound_streak);

    if (last_sound_level >= ALARM_SOUND_LEVEL_THRESHOLD) {
      alarm_sound_streak++;

      if (alarm_sound_streak == ALARM_SOUND_STREAK_THRESHOLD) {
        Serial.println("<<<<<< ALARM HAS STARTED >>>>>>");
        add_event(ALARM_ON);
      }
    } else {
      if (alarm_sound_streak > 0) {
        Serial.println("<<<<<< ALARM HAS STOPPED >>>>>>");
        add_event(ALARM_OFF);
      }

      alarm_sound_streak = 0;
    }

    vTaskDelay(SOUND_CHECK_DELAY_SECONDS * 1000 / portTICK_PERIOD_MS);
  }
}


double calculateSoundLevel() {
  const double BaseLevel = 1000;
  double maxLevel = 0;

  for (int n = 0; n < NUM_SAMPLES; n++) {
    if (maxLevel < adcBuffer[n])
      maxLevel = adcBuffer[n];
  }

  if (fabs(maxLevel) < 10)
    return 0;

  double decibels = 20 * log(maxLevel / BaseLevel);
  return decibels;
}


void initI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = 44100,
    .bits_per_sample =
      I2S_BITS_PER_SAMPLE_16BIT,  // is fixed at 12bit, stereo, MSB
    .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 1, 0)
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
#else
    .communication_format = I2S_COMM_FORMAT_I2S,
#endif
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = 128,
  };

  i2s_pin_config_t pin_config;

#if (ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(4, 3, 0))
  pin_config.mck_io_num = I2S_PIN_NO_CHANGE;
#endif

  pin_config.bck_io_num = I2S_PIN_NO_CHANGE;
  pin_config.ws_io_num = PIN_CLK;
  pin_config.data_out_num = I2S_PIN_NO_CHANGE;
  pin_config.data_in_num = PIN_DATA;

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_set_clk(I2S_NUM_0, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
}


int getBatteryLevel(void) {
  uint16_t vbatData = M5.Axp.GetVbatData();
  return round(((double)vbatData) / 3807.0 * 100);
  return (double)vbatData;
  //double vbat = vbatData * 1.1 / 1000;
  //return 100.0 * ((vbat - 3.0) / (4.07 - 3.0));
}


void initTime() {
  showProgress(0, "* Time: ");
  configTime(TZ_GMT_OFFSET * 3600, 3600, "time.google.com", "pool.ntp.org");
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain local time");
    return;
  }

  Serial.println(&timeinfo, "Local time:  %A, %B %d %Y %H:%M:%S");
  showProgress(0, "set\n");
}

