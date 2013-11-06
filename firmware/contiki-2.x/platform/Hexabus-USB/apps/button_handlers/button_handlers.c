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

#include "button.h"
#include "eeprom_variables.h"
#include <sys/clock.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "dev/watchdog.h"
#include "provisioning.h"
#include "cdc_task.h"

static void button_pressed(uint8_t button, uint8_t released, uint16_t ticks)
{
	if (released) {
		provisioning_master();
	} else {
		provisioning_leds();
	}

	if (ticks > CLOCK_SECOND * BUTTON_LONG_CLICK_MS / 1000) {
		eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0x01);
		watchdog_reboot();
	}
}

static void button_clicked(uint8_t button)
{
	menu_activate();
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
}
