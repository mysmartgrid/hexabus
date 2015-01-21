#include "contiki-conf.h"

#include "sys/clock.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "provisioning.h"
#include "button.h"
#include "value_broadcast.h"
#include "endpoints.h"
#include "endpoint_registry.h"
#include "nvm.h"

#if BUTTON_TOGGLES_RELAY
#include "relay.h"
#else
static void relay_toggle(void)
{
}
#endif

#if METERING_POWER
#include "metering.h"
#else
static bool metering_calibrate(void)
{
	return false;
}
#endif

#if BUTTON_HAS_EID
static char button_pushed = 0;

static enum hxb_error_code read(struct hxb_value* value)
{
	value->v_bool = button_pushed;

	return HXB_ERR_SUCCESS;
}

static const char ep_name[] RODATA = "Hexabus Socket Pushbutton";
ENDPOINT_DESCRIPTOR endpoint_sysbutton = {
	.datatype = HXB_DTYPE_BOOL,
	.eid = EP_BUTTON,
	.name = ep_name,
	.read = read,
	.write = 0
};

ENDPOINT_PROPERTY_DESCRIPTOR prop_sysbutton_name = {
	.datatype = HXB_DTYPE_128STRING,
	.eid = EP_BUTTON,
	.propid = EP_PROP_NAME,
};

static void broadcast_button()
{
	button_pushed = 1;
	broadcast_value(EP_BUTTON);
	button_pushed = 0;
}
#else
static void broadcast_button()
{
}
#endif

static void button_clicked(button_pin_t button)
{
	broadcast_button();
	relay_toggle();
}

static void button_pressed(button_pin_t button, button_pin_t released, uint16_t ticks)
{
	if (metering_calibrate()) {
		return;
	}

	if (released) {
		provisioning_slave();
		broadcast_value(0);
	} else {
		provisioning_leds();
	}

	if (ticks > CLOCK_SECOND * BUTTON_LONG_CLICK_MS / 1000) {
		nvm_write_u8(bootloader_flag, 0x01);
		watchdog_reboot();
	}
}

BUTTON_DESCRIPTOR buttons_system = {
	.port = &BUTTON_PIN,
	.mask = 1 << BUTTON_BIT,

	.activeLevel = !!BUTTON_ACTIVE_LEVEL,

	.click_ticks = CLOCK_SECOND * BUTTON_CLICK_MS / 1000,
	.pressed_ticks = 1 + CLOCK_SECOND * BUTTON_CLICK_MS / 1000,

	.clicked = button_clicked,
	.pressed = button_pressed
};

void button_handlers_init()
{
	BUTTON_REGISTER(buttons_system, 1);
#if BUTTON_HAS_EID
	ENDPOINT_REGISTER(endpoint_sysbutton);
	ENDPOINT_PROPERTY_REGISTER(prop_sysbutton_name);
#endif
}
