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
#include "value_broadcast.h"

#include "presence_detector.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "dev/leds.h"

#include "eeprom_variables.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#if PRESENCE_DETECTOR_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static process_event_t motion_event;
static process_event_t no_motion_event;


static uint8_t presence = 0;

void motion_detected(void) {
    process_post(&presence_detector_process,motion_event,NULL);
}

void no_motion_detected(void) {
    process_post(&presence_detector_process,no_motion_event,NULL);
}

uint8_t presence_active(void) {
    return presence;
}

PROCESS(presence_detector_process, "Monitors the motion detector");

PROCESS_THREAD(presence_detector_process, ev, data) {

    static struct etimer motion_timeout_timer;

    PROCESS_BEGIN();

    motion_event = process_alloc_event();
    no_motion_event = process_alloc_event();

    etimer_set(&motion_timeout_timer, CLOCK_SECOND * (60*ACTIVE_TIME));
    etimer_stop(&motion_timeout_timer);

    while(1) {
        PROCESS_WAIT_EVENT();
        
        if(ev == motion_event) {
            presence = 1;
            etimer_stop(&motion_timeout_timer);

            broadcast_value(26);

            PRINTF("Presence active\n");
        } else if (ev == no_motion_event) {
            etimer_restart(&motion_timeout_timer);
            PRINTF("Presence timer started\n");
        } else if (ev == PROCESS_EVENT_TIMER) {
            presence = 0;

            broadcast_value(26);

            PRINTF("Presence inactive\n");
        }
    }

    PROCESS_END();
}
