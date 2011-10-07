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
#define PRINTF(...) printf(__VA_ARGS__)


void shutter_init(void) {
    PRINTF("Shutter init\n");
    SHUTTER_DDR = ( 0x00 | (1<<SHUTTER_OUT_UP) | (1<<SHUTTER_OUT_DOWN) );
    SHUTTER_PORT |= ( (1<<SHUTTER_BUTTON_UP) | (1<<SHUTTER_BUTTON_DOWN) );

}

void shutter_open(void) {
    PRINTF("Shutter open\n");
    SHUTTER_PORT &= ~(1<<SHUTTER_OUT_DOWN);
    SHUTTER_PORT |= (1<<SHUTTER_OUT_UP);
}

void shutter_close(void) {
    PRINTF("Shutter close\n");
    SHUTTER_PORT &= ~(1<<SHUTTER_OUT_UP);
    SHUTTER_PORT |= (1<<SHUTTER_OUT_DOWN);
}

void shutter_open_full(void) {
//    process_start(&shutter_hexabus_process, "1");
}

void shutter_close_full(void) {
//    process_start(&shutter_hexabus_process. "0");
}

void shutter_stop(void) {
    PRINTF("Shutter stop\n");
    SHUTTER_PORT &= ~( (1<<SHUTTER_OUT_UP) | (1<<SHUTTER_OUT_DOWN) );
}

bool shutter_get_state(void) {
    //TODO
    return false;
}

int shutter_get_state_int(void) {
    //TODO
    return 0;
}

PROCESS(shutter_button_process, "React to attached shutter buttons\n");
//PROCESS(shutter_hexabus_process, "React to hexabus packets");
AUTOSTART_PROCESSES(&shutter_button_process);

/*
PROCESS_THREAD(shutter_hexabus_process, ev, data) {

    shutter_init();

    static struct etimer encoder_timer;

    PROCESS_BEGIN();

    etimer_set(&debounce_timer, CLOCK_SECOND * ENCODER_TIMEOUT / 1000);

    shutter_open();

    while(1) {

        PROCESS_WAIT_EVENT();

        if(ev == PROCESS_EVENT_TIMER) {
            shutter_stop();
            break;
        } else if(ev == ENCODER_EVENT) {
            etimer_restart(&encoder_timer);
        }
    }
}
*/


PROCESS_THREAD(shutter_button_process, ev, data) {

    static struct etimer debounce_timer;

    PROCESS_BEGIN();

    PRINTF("Shutter_button_process started\n");
    while(1) {
        etimer_set(&debounce_timer, CLOCK_SECOND * DEBOUNCE_TIME / 1000);

        if(bit_is_clear(SHUTTER_IN, SHUTTER_BUTTON_UP) || bit_is_clear(SHUTTER_IN, SHUTTER_BUTTON_DOWN)) {
            etimer_restart(&debounce_timer);
            PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&debounce_timer));
            
            if(bit_is_clear(SHUTTER_IN, SHUTTER_BUTTON_UP) && bit_is_clear(SHUTTER_IN, SHUTTER_BUTTON_DOWN)) {
                shutter_stop();
            } else if(bit_is_clear(SHUTTER_IN, SHUTTER_BUTTON_UP))  {
                shutter_open();
            } else if(bit_is_clear(SHUTTER_IN, SHUTTER_BUTTON_DOWN)) {
                shutter_close();
            }

            do {
                PRINTF("Waiting for button release\n");
                etimer_restart(&debounce_timer);
                PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&debounce_timer));
            } while(bit_is_clear(SHUTTER_IN, SHUTTER_BUTTON_UP) || bit_is_clear(SHUTTER_IN, SHUTTER_BUTTON_DOWN));
            shutter_stop();
        }
        PROCESS_PAUSE();
    }

    PROCESS_END();
}

             
                
