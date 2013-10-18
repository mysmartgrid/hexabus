#ifndef HEALTH_H_
#define HEALTH_H_

#include "process.h"

typedef enum {
	HE_OK = 0,
	HE_HARDWARE_DEFECT = 1,
	HE_NO_CONNECTION = 2
} health_status_t;

extern process_event_t health_event;

void health_update(health_status_t bit, char set);

#endif
