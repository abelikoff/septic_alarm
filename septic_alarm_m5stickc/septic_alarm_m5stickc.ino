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




void checkSoundVolumeTask(void *arg) {
  size_t bytesread;

  while (1) {
    i2s_read(I2S_NUM_0, (char *)BUFFER, READ_LEN, &bytesread,
             (100 / portTICK_RATE_MS));
    adcBuffer = (int16_t *)BUFFER;

    last_sound_level = calculateSoundLevel();
    Serial.printf("Sound level: %.1f dB  (streak: %d)\n", last_sound_level, alarm_sound_streak);

    if (last_sound_level >= ALARM_SOUND_LEVEL_THRESHOLD) {
      alarm_sound_streak++;

      if (alarm_sound_streak == ALARM_SOUND_STREAK_THRESHOLD) {
        registerAlarmEvent(true);
      }
    } else {
      if (alarm_sound_streak > 0) {
        registerAlarmEvent(false);
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


void i2sInit() {
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

int getBatteryLevel(void) {
  uint16_t vbatData = M5.Axp.GetVbatData();
  return round(((double)vbatData) / 3807.0 * 100);
  return (double)vbatData;
  //double vbat = vbatData * 1.1 / 1000;
  //return 100.0 * ((vbat - 3.0) / (4.07 - 3.0));
}

