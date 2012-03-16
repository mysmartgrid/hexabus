#include "contiki.h"

#include "hexabus_config.h"

#include "day_night_switch.h"

#include "datetime_service.h"
#include "endpoint_access.h"

#if DAY_NIGHT_SWITCH_DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

//TODO: initial value
static uint8_t is_day = 0;
struct datetime time;
extern process_event_t day_night_switch_pressed_event;
extern process_event_t day_night_switch_released_event;
extern process_event_t day_night_switch_clicked_event;

PROCESS(day_night_switch_process, "day/night switch process");

PROCESS_THREAD(day_night_switch_process, ev, data)
{
	PROCESS_BEGIN();

	PRINTF("day/night switch: init\n");
	struct hxb_value* value = malloc(sizeof(struct hxb_value));
	value->datatype = HXB_DTYPE_UINT8;
	*(uint8_t*)&value->data = 8;
	endpoint_write(27, value); // Set initial day/night signal to night
	free(value);
	// initialize timer
	static struct etimer check_timer;
	etimer_set(&check_timer, CLOCK_SECOND * 5);

	while(1) {
		PROCESS_WAIT_EVENT();

		if(ev == PROCESS_EVENT_TIMER) {
			//Reset time to prevent invalid datetime
			PRINTF("day/night switch: timer event - resetting datetime\n");
			if(is_day) {
				time.hour = DAY_NIGHT_SWITCH_DAY_HOUR;
			} else {
				time.hour = DAY_NIGHT_SWITCH_NIGHT_HOUR;
			}
			time.minute = 0;
			time.second = 0;
			time.day = 1;
			time.month = 1;
			time.year = 1970;
			time.weekday = 4;
			struct hxb_envelope* envelope = malloc(sizeof(struct hxb_envelope));
			memcpy(&(envelope->value.data), &(time), sizeof(struct datetime));
			updateDatetime(envelope);
			etimer_reset(&check_timer);
		}
		if(ev == day_night_switch_pressed_event) {
			PRINTF("day/night switch: pressed event received\n");
			is_day = 1;
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
			struct hxb_value* value = malloc(sizeof(struct hxb_value));
			value->datatype = HXB_DTYPE_UINT8;
			*(uint8_t*)&value->data = 4;
			endpoint_write(27, value); // Set day signal
			free(value);
		} 
		if(ev == day_night_switch_released_event) {
			PRINTF("day/night switch: released event received\n");

			is_day = 0;
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
			struct hxb_value* value = malloc(sizeof(struct hxb_value));
			value->datatype = HXB_DTYPE_UINT8;
			*(uint8_t*)&value->data = 8;
			endpoint_write(27, value); // Set night signal
			free(value);
		}
	}
	PROCESS_END();
}
