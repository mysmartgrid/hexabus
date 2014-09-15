#include "datetime_service.h"
#include "sys/clock.h"
#include "sys/etimer.h"
#include "contiki.h"

#include "hexabus_config.h"
#include "nvm.h"

#include <stdlib.h>
#include <stdio.h>

#define LOG_LEVEL DATETIME_SERVICE_DEBUG
#include "syslog.h"

static uint64_t current_dt;
static bool time_valid;

static struct etimer update_timer;

void updateDatetime(struct hxb_envelope* envelope)
{
	syslog(LOG_DEBUG, "Time: Got update.");

	current_dt = envelope->value.v_u64;
	time_valid = true;

	etimer_restart(&update_timer);
}

int getDatetime(uint64_t* dt)
{
	*dt = current_dt;

	return time_valid ? 0 : -1;
}

PROCESS(datetime_service_process, "Keeps the Date and Time up-to-date\n");

PROCESS_THREAD(datetime_service_process, ev, data)
{
	PROCESS_BEGIN();

	time_valid = false;

	etimer_set(&update_timer, CLOCK_SECOND);

	while(1) {
		PROCESS_WAIT_EVENT();

		if (ev == PROCESS_EVENT_TIMER) {
			syslog(LOG_INFO, "Time: %d:%d:%d\t%d.%d.%d Day: %d Valid: %d",
					current_dt.hour, current_dt.minute, current_dt.second,
					current_dt.day, current_dt.month, current_dt.year, current_dt.weekday, time_valid);

			if (!etimer_expired(&update_timer))
				continue;

			etimer_reset(&update_timer);

#if DTS_SKIP_EVERY_N
			static unsigned skipCounter = 0;
			skipCounter++;
			if (DTS_SKIP_EVERY_N > 0 && skipCounter == DTS_SKIP_EVERY_N) {
				skipCounter = 0;
				continue;
			} else if (DTS_SKIP_EVERY_N < 0 && skipCounter == -DTS_SKIP_EVERY_N) {
				current_dt += 1;
			}
#endif

			current_dt += 1;
		}
	}
	PROCESS_END();
}

