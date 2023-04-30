#ifndef INCLUDED_EVENT_H
#define INCLUDED_EVENT_H

#include <time.h>

enum event_type {
  STARTED,    // device has started
  ALARM_OFF,  // alarm is off
  ALARM_ON    // alarm is on
};


extern void add_event(event_type ev_type);
extern bool peek_event(event_type& ev_type, struct tm& timestamp);
extern bool pop_event(event_type& ev_type, struct tm& timestamp);
extern int get_number_of_events();
extern bool get_last_alarm_time(struct tm& timestamp);

#endif
