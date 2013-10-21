#ifndef HEALTH_H_
#define HEALTH_H_

#include "process.h"

typedef enum {
	HE_OK = 0,
	HE_NO_CONNECTION = 1,
	HE_PT100_BROKEN = 2,
	HE_HYT_BROKEN = 4,

	HE_HARDWARE_DEFECT = HE_PT100_BROKEN | HE_HYT_BROKEN
} health_status_t;

extern process_event_t health_event;

void health_update(health_status_t bit, char set);

#endif
