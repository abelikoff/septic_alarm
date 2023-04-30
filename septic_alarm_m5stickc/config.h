#ifndef INCLUDED_CONFIG_H
#define INCLUDED_CONFIG_H

#ifdef DEBUG

const double ALARM_SOUND_LEVEL_THRESHOLD = 4.0;
const int ALARM_SOUND_STREAK_THRESHOLD = 1;
const int SOUND_CHECK_DELAY_SECONDS = 5;
const int CONN_CHECK_DELAY_SECONDS = 60;        // How often to check WiFi status (seconds)
const int RECONNECT_RETRY_PERIOD_SECONDS = 60;  // How often to re-try reconnecting to WiFi (seconds)
const int EVT_POST_DELAY_SECONDS = 5;        // How often to check for new events to post
const int EVT_POST_RETRY_PERIOD_SECONDS = 180;        // How often to retry posting events after failure
const int EVT_POST_THROTTLE_PERIOD_SECONDS = 30;        // Minimum time between two events posts

#else  // production settings

const double ALARM_SOUND_LEVEL_THRESHOLD = 9.0;
const int ALARM_SOUND_STREAK_THRESHOLD = 4;
const int SOUND_CHECK_DELAY_SECONDS = 30;
const int CONN_CHECK_DELAY_SECONDS = 60;         // How often to check WiFi status (seconds)
const int RECONNECT_RETRY_PERIOD_SECONDS = 300;  // How often to re-try reconnecting to WiFi (seconds)
const int EVT_POST_DELAY_SECONDS = 5;        // How often to check for new events to post
const int EVT_POST_RETRY_PERIOD_SECONDS = 180;        // How often to retry posting events after failure
const int EVT_POST_THROTTLE_PERIOD_SECONDS = 30;        // Minimum time between two events posts

#endif

#endif
