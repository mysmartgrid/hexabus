#ifndef DAY_NIGHT_SWITCH_H
#define DAY_NIGHT_SWITCH_H

#include "process.h"

PROCESS_NAME(day_night_switch_process);


extern process_event_t day_night_switch_pressed_event;
extern process_event_t day_night_switch_released_event;
extern process_event_t day_night_switch_clicked_event;

#endif
