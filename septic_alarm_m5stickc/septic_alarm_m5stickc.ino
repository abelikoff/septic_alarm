#include <cmath>

#include <M5StickCPlus.h>
#include <WiFi.h>
#include <driver/i2s.h>

#include "arduino_secrets.h"

const char *ssid = SECRET_WIFI_SSID;
const char *password = SECRET_WIFI_PASSWORD;

#define PIN_CLK 0
#define PIN_DATA 34
#define READ_LEN (2 * 256)
#define GAIN_FACTOR 3
#define NUM_SAMPLES 160

const double AlarmSoundThreshold = 9.0;

uint8_t BUFFER[READ_LEN] = { 0 };

uint16_t oldy[NUM_SAMPLES];
int16_t *adcBuffer = NULL;
bool alarmIsOn = false;

const TickType_t SoundCheckDelay = 5 * 1000 / portTICK_PERIOD_MS;  // 1 minute delay


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

void displayMAVG(uint16_t mavg) {
  char s[32];
  snprintf(s, sizeof(s), "MAVG: %d", mavg);
  M5.Lcd.drawString(s, 80, 60, 2);
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

void checkSoundVolumeTask(void *arg) {
  size_t bytesread;

  while (1) {
    i2s_read(I2S_NUM_0, (char *)BUFFER, READ_LEN, &bytesread,
             (100 / portTICK_RATE_MS));
    adcBuffer = (int16_t *)BUFFER;

    double level = calculateSoundLevel();
    Serial.printf("Sound level: %.1f dB  (alarm %s on)\n", level, (alarmIsOn ? "is" : "is NOT"));

    if (level >= AlarmSoundThreshold) {
      if (!alarmIsOn) {
        alarmIsOn = true;
        registerAlarmEvent(true);
      }
    } else {
      if (alarmIsOn) {
        alarmIsOn = false;
        registerAlarmEvent(false);
      }
    }

    displayMAVG(level);
    showSignal();
    vTaskDelay(SoundCheckDelay);
  }
}

void registerAlarmEvent(bool started) {
  if (started) {
    Serial.println("+++++++ ALARM HAS STARTED");
  } else {
    Serial.println("------- ALARM HAS STOPPED");
  }
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

void setup() {
  M5.begin();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.drawString("M5Stack has been connected", 160, 120, 2);

  /*M5.Lcd.setRotation(3);
    M5.Lcd.fillScreen(WHITE);
    M5.Lcd.setTextColor(BLACK, WHITE);
    M5.Lcd.println("zz mic test");*/


  Serial.begin(115200);
  delay(1000);

#if 0
  WiFi.mode(WIFI_STA);  //Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
#endif

  i2sInit();
  xTaskCreate(checkSoundVolumeTask, "checkSoundVolumeTask", 2048, NULL, 1, NULL);
}

void loop() {

  M5.update();  // Read the press state of the key.  读取按键 A, B, C 的状态

  if (M5.BtnA.isPressed()) {
      Serial.print(".");

    M5.Lcd.setCursor(0, 0);
    M5.Lcd.print("release Button A, 3 seconds will clear screen");
  }
  if (M5.BtnA.wasReleasefor(3000)) {
          Serial.println("released");

    //M5.Lcd.fillScreen(BLACK);
  }

  vTaskDelay(500 / portTICK_RATE_MS);

/*  if (M5.BtnA.wasReleased()) {  // If the button A is pressed.  如果按键 A 被按下
    M5.Lcd.print('A');
  } else if (M5.BtnB.wasReleased()) {
    M5.Lcd.print('B');
  } else if (M5.BtnB.wasReleasefor(700)) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
  }*/

  //printf("loop cycling\n");
  //vTaskDelay(1000 / portTICK_RATE_MS);  // otherwise the main task wastes half
  // of the cpu cycles
}
