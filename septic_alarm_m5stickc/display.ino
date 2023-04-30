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

#include "event.h"

extern const char *getSSID();


void initDisplay() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(-1);
  M5.Lcd.setTextFont(2);
  M5.Lcd.setTextSize(1);
  //M5.Lcd.setFreeFont(&FreeMonoBold12pt7b);
  M5.Lcd.setTextDatum(TL_DATUM);
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

    auto level = getBatteryLevel();

    if (level < 30) {
      M5.Lcd.setTextColor(RED);
    } else if (level < 60) {
      M5.Lcd.setTextColor(YELLOW);
    } else {
      M5.Lcd.setTextColor(GREEN);
    }

    snprintf(str, sizeof(str), "%d%%", level);
    M5.Lcd.drawString(str, left_margin + prefix_len, 20, font);

    prefix_len = M5.Lcd.textWidth(wlan_prefix, font);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.drawString(wlan_prefix, left_margin, 45, font);
    bool connected;
    snprintf(str, sizeof(str), "%s", getWiFiStatus(connected));

    if (connected) {
      M5.Lcd.setTextColor(GREEN);
      snprintf(str, sizeof(str), "connected (%s)", getSSID());
      M5.Lcd.drawString(str, left_margin + prefix_len, 45, font);

    } else {
      M5.Lcd.setTextColor(RED);
      M5.Lcd.drawString(str, left_margin + prefix_len, 45, font);
    }

    // display last alarm time

    prefix_len = M5.Lcd.textWidth(last_alarm_prefix, font);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.drawString(last_alarm_prefix, left_margin, 70, font);

    struct tm last_alarm_time;

    if (get_last_alarm_time(last_alarm_time)) {
      M5.Lcd.setTextColor(WHITE);
      strftime(str, sizeof(str), "%Y/%m/%d %H:%M:%S", &last_alarm_time);
      M5.Lcd.drawString(str, left_margin + prefix_len, 70, font);
    }

    prefix_len = M5.Lcd.textWidth(sound_prefix, font);
    M5.Lcd.setTextColor(YELLOW);
    M5.Lcd.drawString(sound_prefix, left_margin, 95, font);
    M5.Lcd.setTextColor(WHITE);
    snprintf(str, sizeof(str), "%.1f dB", last_sound_level);
    M5.Lcd.drawString(str, left_margin + prefix_len, 95, font);

    snprintf(str, sizeof(str), "v%s", version);
    prefix_len = M5.Lcd.textWidth(str, font);
    M5.Lcd.drawString(str, M5.Lcd.width() - prefix_len, 120, 1);

    // showBatteryLevel();
  } else {
    M5.Lcd.writecommand(ST7735_DISPOFF);
    M5.Axp.ScreenBreath(0);
  }
}

void showProgress(int level, const char *message) {
  if (level == 0) {
    M5.Lcd.setTextColor(WHITE);
  } else if (level == 1) {
    M5.Lcd.setTextColor(YELLOW);
  } else {
    M5.Lcd.setTextColor(RED);
  }

  M5.Lcd.printf("%s", message);
}
