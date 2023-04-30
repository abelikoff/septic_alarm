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

#ifndef INCLUDED_CONFIG_H
#define INCLUDED_CONFIG_H

//#define DEBUG         // Uncomment for more debug-friendly setup

const int TZ_GMT_OFFSET = -5;                     // Eastern/New_York
const int CONN_CHECK_DELAY_SECONDS = 60;          // How often to check WiFi status (seconds)
const int EVT_POST_DELAY_SECONDS = 5;             // How often to check for new events to post
const int EVT_POST_RETRY_PERIOD_SECONDS = 180;    // How often to retry posting events after failure
const int EVT_POST_THROTTLE_PERIOD_SECONDS = 20;  // Minimum time between two events posts

#ifdef DEBUG

const double ALARM_SOUND_LEVEL_THRESHOLD = 4.0;  // sound level (dB) above it is considered an alarm
const int ALARM_SOUND_STREAK_THRESHOLD = 1;      // how many times should we sample it before considering it an alarm
const int SOUND_CHECK_DELAY_SECONDS = 5;
const int RECONNECT_RETRY_PERIOD_SECONDS = 60;  // How often to re-try reconnecting to WiFi (seconds)

#else  // production settings

const double ALARM_SOUND_LEVEL_THRESHOLD = 9.0;  // sound level (dB) above it is considered an alarm
const int ALARM_SOUND_STREAK_THRESHOLD = 3;      // how many times should we sample it before considering it an alarm
const int SOUND_CHECK_DELAY_SECONDS = 30;
const int RECONNECT_RETRY_PERIOD_SECONDS = 300;  // How often to re-try reconnecting to WiFi (seconds)

#endif

#endif
