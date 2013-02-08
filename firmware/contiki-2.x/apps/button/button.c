/*
 * Copyright (c) 2011, Fraunhofer ESK
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
 * Author: 	Günter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *
 * @(#)$$
 */

#include "button.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h"
#include "contiki.h"
#include "dev/leds.h"
#include "contiki.h"
#include "eeprom_variables.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "dev/watchdog.h"
#include "provisioning.h"

PROCESS(button_pressed_process, "Check for pressed button process");
AUTOSTART_PROCESSES(&button_pressed_process);

extern void button_clicked(void);
extern void button_double_clicked(void);
extern void button_long_pressed(bool released);
extern void button_held(void);

PROCESS_THREAD(button_pressed_process, ev, data)
{
	PROCESS_EXITHANDLER(goto exit);

	static struct etimer button_timer;
	static clock_time_t pressed_at;
#if BUTTON_DOUBLE_CLICK_ENABLED
	static clock_time_t last_click;
	static unsigned long last_click_seconds;
#endif

#define WAIT_FOR_MS(duration) do { \
		etimer_set(&button_timer, CLOCK_SECOND * duration / 1000); \
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&button_timer)); \
	} while (0)

	PROCESS_BEGIN();

	while (1) {
		// BUTTON_PIN is externally pulled low by the actual button
		if (bit_is_clear(BUTTON_PIN, BUTTON_BIT)) {
			WAIT_FOR_MS(BUTTON_DEBOUNCE_MS);

			if (bit_is_clear(BUTTON_PIN, BUTTON_BIT)) {
				pressed_at = clock_time();

				// wait for button release or until the long click threshold is passed
				do {
					WAIT_FOR_MS(BUTTON_DEBOUNCE_MS);
				} while (bit_is_clear(BUTTON_PIN, BUTTON_BIT) && clock_time() - pressed_at < CLOCK_SECOND * BUTTON_CLICK_MS / 1000);

				if (bit_is_clear(BUTTON_PIN, BUTTON_BIT)) {
					// button is still pressed, but has been pressed for longer than the acceptable click time
					// wait for button release or until the button hold threshold is passed
					do {
						button_long_pressed(false);

						WAIT_FOR_MS(BUTTON_DEBOUNCE_MS);
					} while (bit_is_clear(BUTTON_PIN, BUTTON_BIT) && clock_time() - pressed_at < CLOCK_SECOND * BUTTON_LONG_CLICK_MS / 1000);

					if (clock_time() - pressed_at > CLOCK_SECOND * BUTTON_LONG_CLICK_MS / 1000) {
						button_held();

						// eat the button until it is released, otherwise spurious clicks might be triggered
						do {
							WAIT_FOR_MS(BUTTON_DEBOUNCE_MS);
						} while (bit_is_clear(BUTTON_PIN, BUTTON_BIT));
						continue;
					}
				}

				if (bit_is_set(BUTTON_PIN, BUTTON_BIT)) {
					clock_time_t click_time = clock_time() - pressed_at;

					if (click_time >= CLOCK_SECOND * BUTTON_CLICK_MS / 1000) {
						button_long_pressed(true);
					} else if (click_time > CLOCK_SECOND * BUTTON_DOUBLE_CLICK_MS / 1000) {
						button_clicked();
#if BUTTON_DOUBLE_CLICK_ENABLED
						last_click = pressed_at;
						last_click_seconds = clock_seconds();
					} else if (last_click_seconds - clock_seconds() < 2) {
						// clock_time_t may wrap at inopportune times, so this has to be checked
						// also, a double click may have a clock_seconds() transition in the click pause
						button_double_clicked();
						// next click should not trigger another double click, so set last click to a value that will force
						// clock_seconds() - last_click_seconds to exceed the threshold by a large amount
						last_click = -clock_time();
#endif
					}
				}
			}
		}

		WAIT_FOR_MS(BUTTON_DEBOUNCE_MS);
	}

exit: ;

	PROCESS_END();
}

