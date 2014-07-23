#include "datetime_service.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"

#include "hexabus_config.h"
#include "eeprom_variables.h"
#include <avr/eeprom.h>

#include <stdlib.h>
#include <stdio.h>

#define LOG_LEVEL DATETIME_SERVICE_DEBUG
#include "syslog.h"

static struct hxb_datetime current_dt;
static bool time_valid;

static struct etimer update_timer;

void updateDatetime(struct hxb_envelope* envelope)
{
	syslog(LOG_DEBUG, "Time: Got update.");

	current_dt = envelope->value.v_datetime;
	time_valid = true;

	etimer_restart(&update_timer);
}

int getDatetime(struct hxb_datetime *dt)
{
	dt->hour = current_dt.hour;
	dt->minute = current_dt.minute;
	dt->second = current_dt.second;
	dt->day = current_dt.day;
	dt->month = current_dt.month;
	dt->year = current_dt.year;
	dt->weekday = current_dt.weekday;

	return time_valid ? 0 : -1;
}

PROCESS(datetime_service_process, "Keeps the Date and Time up-to-date\n");

static void updateClock()
{
	if (++current_dt.second < 60)
		return;

	current_dt.second = 0;
	if (++current_dt.minute < 60)
		return;

	current_dt.minute = 0;
	if (++current_dt.hour < 24)
		return;

	char monthLength = 31;
	switch (current_dt.month) {
		case 2:
			if ((current_dt.year % 4 == 0 && current_dt.year % 100 != 0) || current_dt.year % 400 == 0)
				monthLength = 29;
			else
				monthLength = 28;
			break;

		case 4:
		case 6:
		case 9:
		case 11:
			monthLength = 30;
			break;
	}

	current_dt.hour = 0;
	current_dt.weekday = (current_dt.weekday + 1) % 7;
	if (++current_dt.day <= monthLength)
		return;

	current_dt.day = 1;
	if (++current_dt.month < 12)
		return;

	current_dt.month = 1;
	current_dt.year++;
}

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
				updateClock();
			}
#endif

			updateClock();
		}
	}
	PROCESS_END();
}

