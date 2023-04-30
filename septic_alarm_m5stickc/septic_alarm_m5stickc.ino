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

#include <cmath>
#include <M5StickCPlus.h>

#include "config.h"
#include "event.h"

const char version[] = "1.0.0";

bool display_on = false;
double last_sound_level = 0;


double calculateSoundLevel();
void connectToInternet(bool display_progress = false);


void setup() {
  M5.begin();
  Serial.begin(115200);
  delay(1000);

  event_buffer_init();
  initDisplay();
  connectToInternet(true);
  initTime();
  add_event(STARTED);
  initI2S();

  // This task samples sound and calculates sound level
  xTaskCreate(checkSoundVolumeTask, "checkSoundVolumeTask", 2048, NULL, 1, NULL);

  // This task periodically checks the connection and handles disconnects
  xTaskCreate(connectionCheckerTask, "connectionCheckerTask", 2048, NULL, 1, NULL);

  // This task processes the event buffer
  xTaskCreate(eventPostingTask, "eventPostingTask", 2048, NULL, 1, NULL);
}


// Loop is only used to read the buttons and show the status
// (all meaningful work is done in 3 tasks)

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
