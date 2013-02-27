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
 * Author: Sean Buckheister <buckheister@itwm.fhg.de>
 *
 */

#ifndef BUTTON_H_
#define BUTTON_H_

#include "hexabus_config.h"
#include "process.h"

#include <avr/pgmspace.h>

PROCESS_NAME(button_pressed_process);

struct button_set_descriptor_t {
	volatile uint8_t* port;
	uint8_t mask;

	uint8_t activeLevel:1;

	// NOTE: tick quantities in this structure are guaranteed to hold at 4096 ticks. Larger
	// values may be clipped to 4096
	
	// transitions idle -> active -> idle that fit into this time frame are clicks
	uint16_t click_ticks;
	// transitions idle -> active -> idle report button state after this many ticks, once per tick
	uint16_t pressed_ticks;

	void (*clicked)(uint8_t button);
	void (*pressed)(uint8_t button, uint8_t released, uint16_t pressed_ticks);
};

#define BUTTON_DESCRIPTOR const struct button_set_descriptor_t PROGMEM

struct button_sm_state_t {
	const struct button_set_descriptor_t* descriptor;
	struct button_sm_state_t* next;

	uint16_t* state;
};

extern struct button_sm_state_t* _button_chain;


#define BUTTON_REGISTER(DESC, pincount) \
	do { \
		static uint16_t state[pincount] = { 0 }; \
		static struct button_sm_state_t _button_state = { 0, 0, state }; \
		_button_state.descriptor = &DESC; \
		_button_state.next = _button_chain; \
		_button_chain = &_button_state; \
	} while (0)

#endif /* BUTTON_H_ */
