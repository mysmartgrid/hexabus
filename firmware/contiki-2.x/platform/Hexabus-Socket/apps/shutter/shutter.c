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
#include "hexabus_config.h"

#include "shutter.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "dev/leds.h"

#include "eeprom_variables.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#include <stdio.h>

#if SHUTTER_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static process_event_t cancel_event;

static int8_t shutter_direction;
static int shutter_goal;
static int shutter_pos;
static int shutter_upperbound;

void shutter_init(void) {
    PRINTF("Shutter init\n");
    SHUTTER_DDR |= ( 0x00 | (1<<SHUTTER_OUT_UP) | (1<<SHUTTER_OUT_DOWN) );
    SHUTTER_DDR &= ~( (1<<PA4) | (1<<PA5) );
    SHUTTER_PORT |= ( (1<<PA4) | (1<<PA5) ); //pull-ups for encoder inputs
    
    /* Enable pin interrupts for encoder ports */
    PCMSK0 |= ( 1<<SHUTTER_ENC1 ); 
    PCMSK0 |= ( 1<<SHUTTER_ENC2 );
    PCICR |= ( 1<<PCIE0 );
    sei();

    cancel_event = process_alloc_event();

    shutter_goal = 0;
    shutter_direction = 0;
    shutter_pos = 1;
    shutter_upperbound = 254;
}

void shutter_open(void) {
    PRINTF("Shutter opening\n");
    shutter_direction = 1;
    SHUTTER_PORT &= ~(1<<SHUTTER_OUT_DOWN);
    SHUTTER_PORT |= (1<<SHUTTER_OUT_UP);
}

void shutter_close(void) {
    PRINTF("Shutter closing\n");
    shutter_direction = -1;
    SHUTTER_PORT &= ~(1<<SHUTTER_OUT_UP);
    SHUTTER_PORT |= (1<<SHUTTER_OUT_DOWN);
}

void shutter_stop(void) {
    PRINTF("Shutter stop\n");
    shutter_direction = 0;
    process_post(&shutter_process, cancel_event, NULL);
    SHUTTER_PORT &= ~( (1<<SHUTTER_OUT_UP) | (1<<SHUTTER_OUT_DOWN) );
}

void shutter_set(uint8_t val) {
    shutter_goal = val * (shutter_upperbound / 254);
    PRINTF("-Moving to %d\n", shutter_goal);
    if(shutter_goal > shutter_pos) {
        shutter_open();
    } else {
        shutter_close();
    }
}

void shutter_toggle(uint8_t val) {
    if(shutter_direction==0 && val!=0){
        shutter_set(val);
    } else {
        shutter_stop();
    }
}

uint8_t shutter_get_state(void) {
    return shutter_pos / 254;
}

PROCESS(shutter_process, "Open/Close shutter until motor stopps");


PROCESS_THREAD(shutter_process, ev, data) {

    static struct etimer encoder_timer;

    PROCESS_BEGIN();

    etimer_set(&encoder_timer, CLOCK_SECOND * ENCODER_TIMEOUT / 1000);

    while(1) {
        PROCESS_WAIT_EVENT();

        if(ev == PROCESS_EVENT_POLL) {
            etimer_restart(&encoder_timer);
            if((shutter_direction == 1 && shutter_pos >= shutter_goal) || (shutter_direction == -1 && shutter_pos <= shutter_goal)) {
                shutter_stop();
                PRINTF("-Position reached");
            }
        } else if(ev == PROCESS_EVENT_TIMER) {
            PRINTF("-Encoder timed out\n");
            if(shutter_direction==1) {
                shutter_upperbound = shutter_pos;
                PRINTF("-New upperbound %d\n", shutter_upperbound);
            } else {
                shutter_pos = 1;
            }
            shutter_stop();
        } else if(ev == cancel_event) {
            PRINTF("-Canceled\n");
        }
    }
    PROCESS_END();
}

ISR(PCINT0_vect) {
    shutter_pos += shutter_direction;
    process_poll(&shutter_process);
}
