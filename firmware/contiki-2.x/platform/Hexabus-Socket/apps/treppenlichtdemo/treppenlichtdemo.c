#include "treppenlichtdemo.h"

#include "datetime_service.h"

#include "contiki.h"

#include "hexabus_config.h"

//static uint8_t is_day = TREPPENHAUSLICHT_IS_DAY;
static struct datetime daytime;
static struct datetime nighttime;

PROCESS_THREAD(treppenlicht_demo_process, ev, data)
{
	PROCESS_BEGIN();

	daytime.hour = 12;
	daytime.minute = 0;
	daytime.second = 0;
	daytime.day = 1;
	daytime.month = 1;
	daytime.year = 1970;
	daytime.weekday = 4;

	nighttime.hour = 23;
	nighttime.minute = 0;
	nighttime.second = 0;
	nighttime.day = 1;
	nighttime.month = 1;
	nighttime.year = 1970;
	nighttime.weekday = 4;

	struct hxb_envelope* envelope = malloc(sizeof(struct hxb_envelope));
	while(1) {
		PROCESS_WAIT_EVENT();


		if(ev == demo_licht_pressed_event) {
			memcpy(&(envelope->value.data), &(daytime), sizeof(struct datetime));
			updateDatetime(envelope);
		} 
		if(ev == demo_licht_released_event) {
			memcpy(&(envelope->value.data), &(nighttime), sizeof(struct datetime));
			updateDatetime(envelope);
		}
	}
	PROCESS_END();
}
