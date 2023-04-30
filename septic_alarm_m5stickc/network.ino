#include <WiFi.h>
#include <time.h>
#include <WiFiUdp.h>
#include "ThingSpeak.h"

#include "arduino_secrets.h"
#include "config.h"
#include "event.h"


const char *ssid = SECRET_WIFI_SSID;
const char *password = SECRET_WIFI_PASSWORD;


void eventPostingTask(void *arg) {
  unsigned long last_successful_post_time = 0;
  unsigned long last_unsuccessful_post_time = 0;

  while (true) {
    auto now = millis();
    double time_since_successfull_post = last_successful_post_time > 0 ? (now - last_successful_post_time) * 1000 : -1;
    double time_since_unsuccessfull_post = last_unsuccessful_post_time > 0 ? (now - last_unsuccessful_post_time) * 1000 : -1;
    bool can_post = true;

    auto wifi_status = WiFi.status();

    if (wifi_status != WL_IDLE_STATUS && wifi_status != WL_CONNECTED) {
      Serial.printf("Postman: CANNOT post (no WiFi)\n");
      can_post = false;
    }

    if (time_since_successfull_post > 0 && time_since_successfull_post < EVT_POST_THROTTLE_PERIOD_SECONDS) {
      Serial.printf("Postman: CANNOT post (only %d seconds passed since last post)\n", (int)time_since_successfull_post);
      can_post = false;
    }

    if (time_since_unsuccessfull_post > 0 && time_since_unsuccessfull_post < EVT_POST_RETRY_PERIOD_SECONDS) {
      Serial.printf("Postman: CANNOT post (only %d seconds passed since last retry)\n", (int)time_since_unsuccessfull_post);
      can_post = false;
    }

    event_type ev_type;
    struct tm ev_time;

    if (!peek_event(ev_type, ev_time)) {
      Serial.printf("Postman: CANNOT post (event buffer empty)\n");
      can_post = false;
    }

    if (can_post) {
      Serial.printf("Postman: about to post\n");

      if (postStatus(ev_type)) {
        pop_event(ev_type, ev_time);
        last_successful_post_time = millis();
        last_unsuccessful_post_time = 0;
      } else {
        last_successful_post_time = 0;
        last_unsuccessful_post_time = millis();
      }
    }

    vTaskDelay(EVT_POST_DELAY_SECONDS * 1000 / portTICK_PERIOD_MS);
  }
}


bool postStatus(event_type st) {
  int x;

  if (st == STARTED) {
    x = ThingSpeak.writeField(THINGSPEAK_CHANNEL_ID, 2, 1, THINGSPEAK_API_KEY);
  } else {
    x = ThingSpeak.writeField(THINGSPEAK_CHANNEL_ID, 1, st == ALARM_ON ? 1 : 0, THINGSPEAK_API_KEY);
  }
  if (x != 200) {
    Serial.println("Failed to update channel. HTTP error code: " + String(x));
    return false;
  }

  return true;
}


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
