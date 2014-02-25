/*
 * Copyright (c) 2013, Fraunhofer ITWM
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *
 * Author: Sean Buckheister <buckheister@itwm.fraunhofer.de>
 *
 */

#include "contiki-conf.h"

#include "hexabus_config.h"
#include "eeprom_variables.h"
#include "sys/clock.h"
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "dev/watchdog.h"
#include "provisioning.h"
#include "metering.h"
#include "button.h"
#include "value_broadcast.h"
#include <endpoints.h>
#include "relay.h"
#include "endpoint_registry.h"

#if BUTTON_HAS_EID
int button_pushed = 0;

static enum hxb_error_code read(struct hxb_value* value)
{
	value->v_bool = button_pushed;

	return HXB_ERR_SUCCESS;
}

static const char ep_name[] PROGMEM = "Hexabus Socket Pushbutton";
ENDPOINT_DESCRIPTOR endpoint_sysbutton = {
	.datatype = HXB_DTYPE_BOOL,
	.eid = EP_BUTTON,
	.name = ep_name,
	.read = read,
	.write = 0
};
#endif

static void button_clicked(uint8_t button)
{
#if BUTTON_HAS_EID
	button_pushed = 1;
	broadcast_value(EP_BUTTON);
	button_pushed = 0;
#endif
#if BUTTON_TOGGLES_RELAY
	relay_toggle();
#endif
}

static void button_pressed(uint8_t button, uint8_t released, uint16_t ticks)
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
		eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0x01);
		watchdog_reboot();
	}
}

BUTTON_DESCRIPTOR buttons_system = {
	.port = &BUTTON_PIN,
	.mask = 1 << BUTTON_BIT,
	
	.activeLevel = 0,

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
#endif
}
