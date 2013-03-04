#include "hexapush.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "button.h"
#include "hexabus_config.h"
#include "value_broadcast.h"
#include "endpoints.h"

#include "endpoint_registry.h"

#include <stdio.h>

#if HEXAPUSH_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


static uint8_t pressed_vector = 0;
static uint8_t clicked_vector = 0;

static prog_char ep_pressed[] = "Pressed Hexapush buttons";
static prog_char ep_clicked[] = "Clicked Hexapush buttons";

enum hxb_error_code read_pressed(uint32_t eid, struct hxb_value* value)
{
	value->v_u8 = pressed_vector;
	return HXB_ERR_SUCCESS;
}

enum hxb_error_code read_clicked(uint32_t eid, struct hxb_value* value)
{
	value->v_u8 = clicked_vector;
	return HXB_ERR_SUCCESS;
}

ENDPOINT_DESCRIPTOR endpoint_hexapush_pressed = {
	.datatype = HXB_DTYPE_UINT8,
	.eid = EP_HEXAPUSH_PRESSED,
	.name = ep_pressed,
	.read = read_pressed,
	.write = 0
};

ENDPOINT_DESCRIPTOR endpoint_hexapush_clicked = {
	.datatype = HXB_DTYPE_UINT8,
	.eid = EP_HEXAPUSH_CLICKED,
	.name = ep_clicked,
	.read = read_clicked,
	.write = 0
};


static void button_clicked(uint8_t button)
{
	PRINTF("Clicked %d\n", button);

	clicked_vector |= button;
	broadcast_value(EP_HEXAPUSH_CLICKED);
	clicked_vector &= ~button;
}

static void button_pressed(uint8_t button, uint8_t released, uint16_t pressed_ticks)
{
	if (!released && !(pressed_vector & button)) {
		PRINTF("Pressed %d\n", button);

		pressed_vector |= button;
		broadcast_value(EP_HEXAPUSH_PRESSED);
	} else if (released && (pressed_vector & button)) {
		PRINTF("Released %d\n", button);

		pressed_vector &= ~button;
		broadcast_value(EP_HEXAPUSH_PRESSED);
	}
}


#define HEXAPUSH_MASK ((HEXAPUSH_B0 << 0) | (HEXAPUSH_B1 << 1) \
		| (HEXAPUSH_B2 << 2) | (HEXAPUSH_B3 << 3) | (HEXAPUSH_B4 << 4) \
		| (HEXAPUSH_B5 << 5) | (HEXAPUSH_B6 << 6) | (HEXAPUSH_B7 << 7))

#define HEXAPUSH_BUTTON_COUNT (!!HEXAPUSH_B0 + !!HEXAPUSH_B1 + !!HEXAPUSH_B2 + !!HEXAPUSH_B3 + !!HEXAPUSH_B4 \
		+ !!HEXAPUSH_B5 + !!HEXAPUSH_B6 + !!HEXAPUSH_B7)

BUTTON_DESCRIPTOR buttons_hexapush = {
	.port = &HEXAPUSH_PIN,
	.mask = HEXAPUSH_MASK,

	.activeLevel = 0,

	.click_ticks = 1,
	.pressed_ticks = HEXAPUSH_CLICK_ENABLE && HEXAPUSH_PRESS_RELEASE_ENABLE ? HEXAPUSH_PRESS_DELAY : 0,

	.clicked = HEXAPUSH_CLICK_ENABLE ? button_clicked : 0,
	.pressed = HEXAPUSH_PRESS_RELEASE_ENABLE ? button_pressed : 0
};

void hexapush_init()
{
	PRINTF("Hexapush init\n");

	HEXAPUSH_DDR &= ~HEXAPUSH_MASK;
	HEXAPUSH_PORT |= HEXAPUSH_MASK;

	BUTTON_REGISTER(buttons_hexapush, HEXAPUSH_BUTTON_COUNT);

	ENDPOINT_REGISTER(endpoint_hexapush_pressed);
	ENDPOINT_REGISTER(endpoint_hexapush_clicked);
}

