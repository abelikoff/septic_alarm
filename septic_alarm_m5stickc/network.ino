#include <WiFi.h>
#include <time.h>
#include <WiFiUdp.h>
#include "ThingSpeak.h"
#include "arduino_secrets.h"


const char *ssid = SECRET_WIFI_SSID;
const char *password = SECRET_WIFI_PASSWORD;


void connectionCheckerTask(void *arg) {
  auto last_reconnect_attempt = millis();

  while (true) {
    auto status = WiFi.status();
    bool wifi_connected = (status == WL_IDLE_STATUS || status == WL_CONNECTED);
    Serial.printf("@@@ Checking WiFi: %s\n", wifi_connected ? "CONNECTED" : "NOT CONNECTED");

    if (!wifi_connected) {
      auto since = millis() - last_reconnect_attempt;
      Serial.printf("WiFi is not connected (%.1f since last attempt)\n", since * 1000.0);

      if (since >= RECONNECT_RETRY_PERIOD_SECONDS * 1000) {
        connectToWiFi();
        last_reconnect_attempt = millis();
      }
    }

    vTaskDelay(CONN_CHECK_DELAY_SECONDS * 1000 / portTICK_PERIOD_MS);
  }
}


void connectToWiFi(bool display_progress) {
  char s[128];

  if (display_progress) {
    snprintf(s, sizeof(s), "* WiFi (%s)... ", ssid);
    showProgress(0, s);
  }

  WiFi.mode(WIFI_STA);  //Optional
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");
  bool connected = false;

  for (int i = 0; i < 100; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }

    Serial.print(".");
    delay(200);
  }

  if (!connected) {
    if (display_progress) {
      showProgress(2, "failed\n");
    }

    Serial.println("Cannot connect to Wi-Fi");
    return;
  }

  Serial.println("");

  if (display_progress) {
    snprintf(s, sizeof(s), "OK (%s)\n", WiFi.localIP().toString());
    showProgress(0, s);
  }
}


void connectToCloud() {
  showProgress(0, "* ThingSpeak... ");
  ThingSpeak.begin(wifi_client);
  showProgress(0, "OK\n");
}


void postStatus(Status st) {
  int x;

  if (st == STARTED) {
    x = ThingSpeak.writeField(THINGSPEAK_CHANNEL_ID, 2, 1, THINGSPEAK_API_KEY);
  } else {
    x = ThingSpeak.writeField(THINGSPEAK_CHANNEL_ID, 1, st == ALARM_ON ? 1 : 0, THINGSPEAK_API_KEY);
  }
  if (x != 200) {
    Serial.println("Failed to update channel. HTTP error code: " + String(x));
  }
}
