#include "treppenlichtdemo.h"

#include "datetime_service.h"

#include "contiki.h"

#include "hexabus_config.h"

static uint8_t is_day = TREPPENHAUSLICHT_IS_DAY;
static struct datetime daytime;
static struct datetime nighttime;

daytime->hour = 12;
daytime->minute = 0;
daytime->second = 0;
daytime->day = 1;
daytime->month = 1;
daytime->year = 1970;
daytime->weekday = 4;

nighttime->hour = 23;
nighttime->minute = 0;
nighttime->second = 0;
nighttime->day = 1;
nighttime->month = 1;
nighttime->year = 1970;
nighttime->weekday = 4;

struct hxb_value val;

PROCESS_THREAD(state_machine_process, ev, data)
{
	PROCESS_BEGIN();
	while(1) {
		PROCESS_WAIT_EVENT();


		if(ev == demo_licht_pressed_event) {
			updateDatetime(daytime);
		} 
		if(ev == demo_licht_released_event) {
			updateDatetime(nighttime);
		}
	}
	PROCESS_END();
}
