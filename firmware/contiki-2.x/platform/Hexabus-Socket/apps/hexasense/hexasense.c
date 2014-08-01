#include "hexasense.h"
#include "button.h"
#include "hexabus_config.h"
#include "value_broadcast.h"
#include "endpoints.h"
#include "sys/etimer.h"

#include "endpoint_registry.h"

#define LOG_LEVEL HEXASENSE_DEBUG
#include "syslog.h"

static uint8_t button_state = 0;

static const char ep_state[] PROGMEM = "HexaSense button state";

enum hxb_error_code read_state(struct hxb_value* value)
{
	value->v_u8 = button_state;
	return HXB_ERR_SUCCESS;
}

ENDPOINT_DESCRIPTOR endpoint_hexasense_state = {
	.datatype = HXB_DTYPE_UINT8,
	.eid = EP_HEXASENSE_BUTTON_STATE,
	.name = ep_state,
	.read = read_state,
	.write = 0
};

ENDPOINT_PROPERTY_DESCRIPTOR prop_hexasense_state_name = {
	.datatype = HXB_DTYPE_128STRING,
	.eid = EP_HEXASENSE_BUTTON_STATE,
	.propid = EP_PROP_NAME,
};

static void button1_pressed(uint8_t button, uint8_t released, uint16_t pressed_ticks)
{
	if (released) {
		button_state ^= 1;
		broadcast_value(EP_HEXASENSE_BUTTON_STATE);
	}
}

static void button2_pressed(uint8_t button, uint8_t released, uint16_t pressed_ticks)
{
	if (released) {
		button_state ^= 2;
		broadcast_value(EP_HEXASENSE_BUTTON_STATE);
	}
}


BUTTON_DESCRIPTOR buttons_hexasense1 = {
	.port = &HEXASENSE_BUTTON1_PORT,
	.mask = 1 << HEXASENSE_BUTTON1_PIN,

	.activeLevel = 0,

	.click_ticks = 0,
	.pressed_ticks = 0,

	.clicked = 0,
	.pressed = button1_pressed
};

BUTTON_DESCRIPTOR buttons_hexasense2 = {
	.port = &HEXASENSE_BUTTON2_PORT,
	.mask = 1 << HEXASENSE_BUTTON2_PIN,

	.activeLevel = 0,

	.click_ticks = 0,
	.pressed_ticks = 0,

	.clicked = 0,
	.pressed = button2_pressed
};

PROCESS(hexasense_feedback_process, "HexaSense user feedback process");

PROCESS_THREAD(hexasense_feedback_process, ev, data)
{
	PROCESS_EXITHANDLER(goto exit);

	static struct etimer timer;

	PROCESS_BEGIN();

	while (1) {
		if (button_state) {
			HEXASENSE_LED1_PORT &= ~(1 << HEXASENSE_LED1_PIN);
			HEXASENSE_LED2_PORT &= ~(1 << HEXASENSE_LED2_PIN);
			HEXASENSE_LED3_PORT &= ~(1 << HEXASENSE_LED3_PIN);
			etimer_set(&timer, CLOCK_CONF_SECOND / 20);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
			HEXASENSE_LED1_PORT |= 1 << HEXASENSE_LED1_PIN;
			HEXASENSE_LED2_PORT |= 1 << HEXASENSE_LED2_PIN;
			HEXASENSE_LED3_PORT |= 1 << HEXASENSE_LED3_PIN;
			etimer_set(&timer, 3 * CLOCK_CONF_SECOND);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
		} else {
			etimer_set(&timer, CLOCK_CONF_SECOND);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
		}
	}

exit: ;

	PROCESS_END();
}

void hexasense_init()
{
	HEXASENSE_LED1_DDR |= 1 << HEXASENSE_LED1_PIN;
	HEXASENSE_LED2_DDR |= 1 << HEXASENSE_LED2_PIN;
	HEXASENSE_LED3_DDR |= 1 << HEXASENSE_LED3_PIN;
	HEXASENSE_LED1_PORT |= 1 << HEXASENSE_LED1_PIN;
	HEXASENSE_LED2_PORT |= 1 << HEXASENSE_LED2_PIN;
	HEXASENSE_LED3_PORT |= 1 << HEXASENSE_LED3_PIN;

	HEXASENSE_BUTTON1_DDR &= ~(1 << HEXASENSE_BUTTON1_PIN);
	HEXASENSE_BUTTON1_PORT |= (1 << HEXASENSE_BUTTON1_PIN);
	HEXASENSE_BUTTON2_DDR &= ~(1 << HEXASENSE_BUTTON2_PIN);
	HEXASENSE_BUTTON2_PORT |= (1 << HEXASENSE_BUTTON2_PIN);

	BUTTON_REGISTER(buttons_hexasense1, 1);
	BUTTON_REGISTER(buttons_hexasense2, 1);

	ENDPOINT_REGISTER(endpoint_hexasense_state);
	ENDPOINT_PROPERTY_REGISTER(prop_hexasense_state_name);

	process_start(&hexasense_feedback_process, NULL);
}

