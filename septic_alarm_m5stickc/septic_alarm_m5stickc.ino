#include <cmath>

#include <M5StickCPlus.h>
#include <utility/ST7735_Defines.h>
#include <WiFi.h>
#include <time.h>
#include "ThingSpeak.h"
#include <driver/i2s.h>

#include "config.h"
#include "event.h"
#include "battery.h"

const char version[] = "0.0.2";

#define PIN_CLK 0
#define PIN_DATA 34
#define READ_LEN (2 * 256)
#define GAIN_FACTOR 3
#define NUM_SAMPLES 160


int alarm_sound_streak = 0;
uint8_t BUFFER[READ_LEN] = { 0 };
uint16_t oldy[NUM_SAMPLES];
int16_t *adcBuffer = NULL;
bool display_on = false;
double last_sound_level = 0;
WiFiClient wifi_client;


double calculateSoundLevel();
void connectToWiFi(bool display_progress = false);


void setup() {
  M5.begin();
  Serial.begin(115200);
  delay(1000);

  initProgress();
  connectToWiFi(true);
  connectToCloud();
  configTime();
  postStatus(STARTED);
  i2sInit();
  xTaskCreate(checkSoundVolumeTask, "checkSoundVolumeTask", 2048, NULL, 1, NULL);
  xTaskCreate(connectionCheckerTask, "connectionCheckerTask", 2048, NULL, 1, NULL);
  xTaskCreate(eventPostingTask, "eventPostingTask", 2048, NULL, 1, NULL);
}


void loop() {
  M5.update();  // Read the press state of the keys

  if (M5.BtnA.wasReleased()) {
    if (!display_on) {
      display_on = true;
      displayStatus(display_on);
    } else {
      display_on = false;
      displayStatus(display_on);
    }
  }

  vTaskDelay(5 / portTICK_RATE_MS);
}


void configTime() {
  showProgress(0, "* Time: ");
  configTime(-5 * 3600, 3600, "time.google.com", "pool.ntp.org");
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain local time");
    return;
  }

  Serial.println(&timeinfo, "Local time:  %A, %B %d %Y %H:%M:%S");
  showProgress(0, "set\n");
}


