#ifndef TREPPENLICHTDEMO_H_
#define TREPPENLICHTDEMO_H_

#include "process.h"

PROCESS_NAME(treppenlicht_demo_process);


extern process_event_t demo_licht_pressed_event;
extern process_event_t demo_licht_released_event;
extern process_event_t demo_licht_clicked_event;

#endif
