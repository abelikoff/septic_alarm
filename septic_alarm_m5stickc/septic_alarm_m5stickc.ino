#include <cmath>

//#include <M5Stack.h>
#include <M5StickCPlus.h>
#include <utility/ST7735_Defines.h>
#include <WiFi.h>
#include <driver/i2s.h>

#include "arduino_secrets.h"

const char *ssid = SECRET_WIFI_SSID;
const char *password = SECRET_WIFI_PASSWORD;
const char version[] = "0.0.1";

#define PIN_CLK 0
#define PIN_DATA 34
#define READ_LEN (2 * 256)
#define GAIN_FACTOR 3
#define NUM_SAMPLES 160

const double ALARM_SOUND_LEVEL_THRESHOLD = 9.0;
const int ALARM_SOUND_STREAK_THRESHOLD = 5;
const int SOUND_CHECK_DELAY_SECONDS = 5;

int alarm_sound_streak = 0;
uint8_t BUFFER[READ_LEN] = { 0 };
uint16_t oldy[NUM_SAMPLES];
int16_t *adcBuffer = NULL;
bool display_on = false;
double last_sound_level = 0;

double calculateSoundLevel();


void setup() {
  M5.begin();
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


void registerAlarmEvent(bool started) {
  if (started) {
    Serial.println("+++++++ ALARM HAS STARTED");
  } else {
    Serial.println("------- ALARM HAS STOPPED");
  }
}


void displayStatus(bool on) {
  if (on) {
    const char wlan_prefix[] = "WLAN: ";
    const char battery_prefix[] = "Bat: ";
    const char last_alarm_prefix[] = "Alarm: ";
    const char sound_prefix[] = "Sound: ";
    const uint16_t left_margin = 10;
    const uint16_t font = 2;
    const uint16_t font_size = 1;
    uint16_t prefix_len;
    char str[64];

    //M5.Lcd.setBrightness(brightness);
    M5.Lcd.writecommand(ST7735_DISPON);
    M5.Axp.ScreenBreath(255);
    //M5.Lcd.wakeup();
    //M5.Lcd.setBrightness(200);
    //M5.Lcd.clear(BLACK);
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setRotation(-1);
    M5.Lcd.setTextFont(font);
    M5.Lcd.setTextSize(font_size);
    //M5.Lcd.setFreeFont(&FreeMonoBold12pt7b);
    M5.Lcd.setTextDatum(TL_DATUM);


    prefix_len = M5.Lcd.textWidth(battery_prefix, font);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.drawString(battery_prefix, left_margin, 20, font);
    M5.Lcd.setTextColor(WHITE);
    snprintf(str, sizeof(str), "80%%");
    M5.Lcd.drawString(str, left_margin + prefix_len, 20, font);

    prefix_len = M5.Lcd.textWidth(wlan_prefix, font);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.drawString(wlan_prefix, left_margin, 45, font);
    M5.Lcd.setTextColor(WHITE);
    snprintf(str, sizeof(str), "Conn");
    M5.Lcd.drawString(str, left_margin + prefix_len, 45, font);

    prefix_len = M5.Lcd.textWidth(last_alarm_prefix, font);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.drawString(last_alarm_prefix, left_margin, 70, font);
    M5.Lcd.setTextColor(WHITE);
    snprintf(str, sizeof(str), "07/28 13:33");
    M5.Lcd.drawString(str, left_margin + prefix_len, 70, font);

    prefix_len = M5.Lcd.textWidth(sound_prefix, font);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.drawString(sound_prefix, left_margin, 95, font);
    M5.Lcd.setTextColor(WHITE);
    snprintf(str, sizeof(str), "%.1f dB", last_sound_level);
    M5.Lcd.drawString(str, left_margin + prefix_len, 95, font);

    snprintf(str, sizeof(str), "v%s", version);
    prefix_len = M5.Lcd.textWidth(str, font);
    M5.Lcd.drawString(str, M5.Lcd.width() - prefix_len, 120, 1);
  } else {
    M5.Lcd.writecommand(ST7735_DISPOFF);
    M5.Axp.ScreenBreath(0);
    //M5.Lcd.sleep();
    //M5.Lcd.setBrightness(0);
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
