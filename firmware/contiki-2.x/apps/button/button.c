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
 * Author: Sean Buckheister <buckheister@itwm.fhg.de>
 *
 */

#include "button.h"

#include "sys/clock.h"
#include "sys/etimer.h"
#include "contiki.h"
#include "contiki.h"
#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>

PROCESS(button_pressed_process, "Button handler process");
AUTOSTART_PROCESSES(&button_pressed_process);

struct button_sm_state_t* _button_chain = 0;

enum button_state_t {
	IDLE = 0,
	DEBOUNCE_ACTIVE = 1,
	ACTIVE = 2,
	DEBOUNCE_IDLE = 3,

	STATE_MASK = 3,
	STATE_SHAMT = 2
};

static void button_iterate_once(void)
{
	for (const struct button_sm_state_t* state = _button_chain; state; state = state->next) {
		uint8_t index = 0;
		struct button_set_descriptor_t d;
		memcpy_P(&d, state->descriptor, sizeof(d));
		for (uint8_t bit = 0x80; bit != 0; bit >>= 1) {
			if (d.mask & bit) {
				uint8_t level = *d.port & bit;
				uint8_t activeLevel = (level != 0 && d.activeLevel != 0)
					|| (level == 0 && d.activeLevel == 0);

				uint16_t ticks = (state->state[index] >> STATE_SHAMT);
				if (((ticks + BUTTON_DEBOUNCE_TICKS) & (0xFFFF >> STATE_SHAMT)) > ticks) {
					ticks += BUTTON_DEBOUNCE_TICKS;
				} else {
					ticks = 0xFFFF >> STATE_SHAMT;
				}
				uint16_t smState = state->state[index] & STATE_MASK;
				switch (smState) {
					case IDLE:
						if (activeLevel) {
							smState = DEBOUNCE_ACTIVE;
							ticks = 0;
						}
						break;

					case DEBOUNCE_ACTIVE:
					case ACTIVE:
						if (activeLevel) {
							smState = ACTIVE;
							if (d.pressed && d.pressed_ticks <= ticks) {
								d.pressed(bit, 0, ticks);
							}
						} else if (smState == DEBOUNCE_ACTIVE) {
							smState = IDLE;
							ticks = 0;
						} else if (smState == ACTIVE) {
							smState = DEBOUNCE_IDLE;
						}
						break;

					case DEBOUNCE_IDLE:
						if (activeLevel) {
							smState = ACTIVE;
						} else {
							if (d.clicked && d.click_ticks >= ticks) {
								d.clicked(bit);
							}
							if (d.pressed && d.pressed_ticks <= ticks) {
								d.pressed(bit, 1, ticks);
							}
							smState = IDLE;
							ticks = 0;
						}
						break;
				}
				state->state[index] = (ticks << 2) | smState;
				index++;
			}
		}
	}
}

PROCESS_THREAD(button_pressed_process, ev, data)
{
	PROCESS_EXITHANDLER(goto exit);

	static struct etimer button_timer;

	PROCESS_BEGIN();

	while (1) {
		button_iterate_once();
		etimer_set(&button_timer, BUTTON_DEBOUNCE_TICKS);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&button_timer));
	}

exit: ;

	PROCESS_END();
}

