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

#include "event.h"

const int EVENT_BUFFER_SIZE = 8;

struct event_info {
  event_type ev_type;
  struct tm ev_time;
} event_buffer[EVENT_BUFFER_SIZE];


int first_event_idx = 0;            // index of the first available event (when non-empty)
int next_event_idx = 0;             // index of the element to store the next event into
int number_of_events = 0;           // number of actual events in the buffer
struct tm last_alarm_time = { 0 };  // timestamp of the last alarm
SemaphoreHandle_t buf_mutex;


void event_buffer_init() {
  buf_mutex = xSemaphoreCreateMutex();
}


void add_event(event_type ev_type) {
  buf_debug("add");
  mutex_lock();
  event_buffer[next_event_idx].ev_type = ev_type;

  if (getLocalTime(&event_buffer[next_event_idx].ev_time)) {
    if (ev_type == ALARM_ON)
      last_alarm_time = event_buffer[next_event_idx].ev_time;
  } else {
    Serial.println("ERROR: failed to obtain local time");
  }

  next_event_idx = (next_event_idx + 1) % EVENT_BUFFER_SIZE;

  if (number_of_events == EVENT_BUFFER_SIZE) {  // we just overwrote the earliest event
    first_event_idx = (first_event_idx + 1) % EVENT_BUFFER_SIZE;
  } else {
    number_of_events++;
  }

  mutex_unlock();
}


bool peek_event(event_type& ev_type, struct tm& timestamp) {
  buf_debug("peek");
  mutex_lock();

  if (number_of_events == 0) {
    mutex_unlock();
    return false;
  }

  ev_type = event_buffer[first_event_idx].ev_type;
  timestamp = event_buffer[first_event_idx].ev_time;
  mutex_unlock();
  return true;
}


bool pop_event(event_type& ev_type, struct tm& timestamp) {
  buf_debug("pop");
  mutex_lock();

  if (number_of_events == 0) {
    mutex_unlock();
    return false;
  }

  ev_type = event_buffer[first_event_idx].ev_type;
  timestamp = event_buffer[first_event_idx].ev_time;
  first_event_idx = (first_event_idx + 1) % EVENT_BUFFER_SIZE;
  number_of_events--;
  mutex_unlock();
  return true;
}


int get_number_of_events() {
  return number_of_events;
}


bool get_last_alarm_time(struct tm& timestamp) {
  if (last_alarm_time.tm_year == 0)
    return false;

  timestamp = last_alarm_time;
  return true;
}


static void mutex_lock() {
  xSemaphoreTake(buf_mutex, portMAX_DELAY);
}

static void mutex_unlock() {
  xSemaphoreGive(buf_mutex);
}

static void buf_debug(const char* s) {
  Serial.printf("Event buffer: <[%d] %d->%d>: %s\n", number_of_events, first_event_idx, next_event_idx, s);
}
