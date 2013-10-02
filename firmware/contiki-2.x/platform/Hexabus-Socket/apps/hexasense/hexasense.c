#include "hexasense.h"
#include "button.h"
#include "hexabus_config.h"
#include "value_broadcast.h"
#include "endpoints.h"

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


static void button1_pressed(uint8_t button, uint8_t released, uint16_t pressed_ticks)
{
	if (released) {
		button_state ^= 1;
	}
}

static void button2_pressed(uint8_t button, uint8_t released, uint16_t pressed_ticks)
{
	if (released) {
		button_state ^= 2;
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

void hexasense_init()
{
	HEXASENSE_BUTTON1_DDR &= ~(1 << HEXASENSE_BUTTON1_PIN);
	HEXASENSE_BUTTON1_PORT |= (1 << HEXASENSE_BUTTON1_PIN);
	HEXASENSE_BUTTON2_DDR &= ~(1 << HEXASENSE_BUTTON2_PIN);
	HEXASENSE_BUTTON2_PORT |= (1 << HEXASENSE_BUTTON2_PIN);

	BUTTON_REGISTER(buttons_hexasense1, 1);
	BUTTON_REGISTER(buttons_hexasense2, 1);

	ENDPOINT_REGISTER(endpoint_hexasense_state);
}

