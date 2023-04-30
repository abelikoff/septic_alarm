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

#ifndef INCLUDED_EVENT_H
#define INCLUDED_EVENT_H

#include <time.h>

enum event_type {
  STARTED,    // device has started
  ALARM_OFF,  // alarm is off
  ALARM_ON    // alarm is on
};


extern void event_buffer_init();
extern void add_event(event_type ev_type);
extern bool peek_event(event_type& ev_type, struct tm& timestamp);
extern bool pop_event(event_type& ev_type, struct tm& timestamp);
extern int get_number_of_events();
extern bool get_last_alarm_time(struct tm& timestamp);

#endif
