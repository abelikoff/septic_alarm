#include <cmath>

#include <M5StickCPlus.h>
#include <utility/ST7735_Defines.h>
#include <WiFi.h>
#include <time.h>
#include "ThingSpeak.h"
#include <driver/i2s.h>

#include "battery.h"

const char version[] = "0.0.2";

#define PIN_CLK 0
#define PIN_DATA 34
#define READ_LEN (2 * 256)
#define GAIN_FACTOR 3
#define NUM_SAMPLES 160

#define DEBUG

#ifdef DEBUG

const double ALARM_SOUND_LEVEL_THRESHOLD = 4.0;
const int ALARM_SOUND_STREAK_THRESHOLD = 1;
const int SOUND_CHECK_DELAY_SECONDS = 5;
const int CONN_CHECK_DELAY_SECONDS = 60;        // How often to check WiFi status (seconds)
const int RECONNECT_RETRY_PERIOD_SECONDS = 60;  // How often to re-try reconnecting to WiFi (seconds)

#else  // production settings

const double ALARM_SOUND_LEVEL_THRESHOLD = 9.0;
const int ALARM_SOUND_STREAK_THRESHOLD = 4;
const int SOUND_CHECK_DELAY_SECONDS = 30;
const int CONN_CHECK_DELAY_SECONDS = 60;         // How often to check WiFi status (seconds)
const int RECONNECT_RETRY_PERIOD_SECONDS = 300;  // How often to re-try reconnecting to WiFi (seconds)

#endif

int alarm_sound_streak = 0;
uint8_t BUFFER[READ_LEN] = { 0 };
uint16_t oldy[NUM_SAMPLES];
int16_t *adcBuffer = NULL;
bool display_on = false;
double last_sound_level = 0;
bool has_last_alarm_timestamp = false;
struct tm last_alarm_tm;
WiFiClient wifi_client;

enum Status {
  STARTED,
  ALARM_OFF,
  ALARM_ON
};

enum ConnStatus {
  ST_CONNECTED,
  ST_RECONNECTED,
  ST_DISCONNECTED,
  ST_NOT_CONNECTED
};


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


void showSignal() {
  int y;

  for (int n = 0; n < NUM_SAMPLES; n++) {
    y = adcBuffer[n] * GAIN_FACTOR;
    y = map(y, INT16_MIN, INT16_MAX, 10, 70);
    M5.Lcd.drawPixel(n, oldy[n], BLACK);
    M5.Lcd.drawPixel(n, y, YELLOW);
    oldy[n] = y;
  }
}


void registerAlarmEvent(bool started) {
  if (started) {
    Serial.println("+++++++ ALARM HAS STARTED");

    if (!getLocalTime(&last_alarm_tm)) {
      Serial.println("Failed to obtain local time");
      has_last_alarm_timestamp = false;
    } else {
      has_last_alarm_timestamp = true;
    }

    postStatus(ALARM_ON);
  } else {
    Serial.println("------- ALARM HAS STOPPED");
    postStatus(ALARM_OFF);
  }
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


const char *getWiFiStatus(bool &connected) {
  connected = false;
  wl_status_t status = WiFi.status();

  switch (status) {
    case WL_IDLE_STATUS: return "idle";
    case WL_NO_SSID_AVAIL: return "no SSID";
    case WL_CONNECTED: connected = true; return "connected";
    case WL_CONNECT_FAILED: return "connection failed";
    case WL_CONNECTION_LOST: return "connection lost";
    case WL_DISCONNECTED: return "disconnected";

    default:
      return "<other>";
  }
}
