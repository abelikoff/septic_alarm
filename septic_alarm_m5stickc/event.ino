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
}


bool peek_event(event_type& ev_type, struct tm& timestamp) {
  if (number_of_events == 0)
    return false;

  ev_type = event_buffer[first_event_idx].ev_type;
  timestamp = event_buffer[first_event_idx].ev_time;
  return true;
}


bool pop_event(event_type& ev_type, struct tm& timestamp) {
  if (!peek_event(ev_type, timestamp))
    return false;

  first_event_idx = (first_event_idx + 1) % EVENT_BUFFER_SIZE;
  number_of_events--;
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

}

static void mutex_unlock() {

}
