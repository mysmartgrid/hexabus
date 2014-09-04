#include "health.h"

process_event_t health_event;

void health_update(health_status_t bit, char set)
{
	static health_status_t status = HE_OK;

	health_status_t old = status;
	if (set) {
		status |= bit;
	} else {
		status &= ~bit;
	}
	if (status != old) {
		process_post(PROCESS_BROADCAST, health_event, &status);
	}
}
