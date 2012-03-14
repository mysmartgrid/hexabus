#include "treppenlichtdemo.h"

#include "datetime_service.h"

#include "contiki.h"

#include "hexabus_config.h"

//static uint8_t is_day = TREPPENHAUSLICHT_IS_DAY;
struct datetime time;
extern process_event_t demo_licht_pressed_event;
extern process_event_t demo_licht_released_event;
extern process_event_t demo_licht_clicked_event;

PROCESS(treppenlicht_demo_process, "Treppenlicht Demo Process");
AUTOSTART_PROCESSES(&treppenlicht_demo_process);

PROCESS_THREAD(treppenlicht_demo_process, ev, data)
{
	printf("tld: init\n");
	PROCESS_BEGIN();

	while(1) {
		PROCESS_WAIT_EVENT();
//TODO: Reset time to prevent invalid datetime

		if(ev == demo_licht_pressed_event) {
			printf("tld: pressed event received\n");
			time.hour = 12;
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
		if(ev == demo_licht_released_event) {
			printf("tld: released event received\n");

			time.hour = 22;
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
