#include "day_night_switch.h"

#include "datetime_service.h"

#include "contiki.h"

#include "hexabus_config.h"

#if DAY_NIGHT_SWITCH_DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

//TODO: initial value
//static uint8_t is_day = TREPPENHAUSLICHT_IS_DAY;
struct datetime time;
extern process_event_t day_night_switch_pressed_event;
extern process_event_t day_night_switch_released_event;
extern process_event_t day_night_switch_clicked_event;

PROCESS(day_night_switch_process, "day/night switch process");
AUTOSTART_PROCESSES(&day_night_switch_process);

PROCESS_THREAD(day_night_switch_process, ev, data)
{
	PRINTF("day/night switch: init\n");
	PROCESS_BEGIN();

	while(1) {
		PROCESS_WAIT_EVENT();
		//TODO: Reset time to prevent invalid datetime

		if(ev == day_night_switch_pressed_event) {
			PRINTF("day/night switch: pressed event received\n");
			time.hour = DAY_NIGHT_SWITCH_DAY_HOUR;
			time.minute = 0;
			time.second = 0;
			time.day = 1;
			time.month = 1;
			time.year = 1970;
			time.weekday = 4;
			struct hxb_envelope* envelope = malloc(sizeof(struct hxb_envelope));
			memcpy(&(envelope->value.data), &(time), sizeof(struct datetime));
			updateDatetime(envelope);
		} 
		if(ev == day_night_switch_released_event) {
			PRINTF("day/night switch: released event received\n");

			time.hour = DAY_NIGHT_SWITCH_NIGHT_HOUR;
			time.minute = 0;
			time.second = 0;
			time.day = 1;
			time.month = 1;
			time.year = 1970;
			time.weekday = 4;
			struct hxb_envelope* envelope = malloc(sizeof(struct hxb_envelope));
			memcpy(&(envelope->value.data), &(time), sizeof(struct datetime));
			updateDatetime(envelope);
		}
	}
	PROCESS_END();
}
