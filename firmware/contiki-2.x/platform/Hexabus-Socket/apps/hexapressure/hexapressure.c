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

#include "hexapressure.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "dev/leds.h"

#include "eeprom_variables.h"
#include <avr/eeprom.h>
#include <avr/interrupt.h>

#if HEXAPRESSURE_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static struct etimer pressure_periodic_timer;

PROCESS(hexapressure_process, "Monitors the pressure sensor");


void hexapressure_init(void) {
    PRINTF("Hexapressure init\n");

    HEXAPRESSURE_DDR |= (1<<SDA);
    HEXAPRESSURE_DDR &= ~(1<<SCL);
    HEXAPRESSURE_PORT |= (1<<SDA);
    HEXAPRESSURE_PORT |= (1<<SCL);
}

PROCESS_THREAD(hexapressure_process, ev, data) {
    PROCESS_BEGIN();
    PRINTF("pressure: process startup.\r\n");
    PROCESS_PAUSE();

    // init the sensor
    hexapressure_init();

    // set the timer to 10 sec for use in the loop
    etimer_set(&pressure_periodic_timer, 10*CLOCK_SECOND);

    //everytime the timer event appears, get the pressure and reset the timer
    while(1){
      PROCESS_YIELD();
      if (etimer_expired(&pressure_periodic_timer)) {
        etimer_reset(&pressure_periodic_timer);
        //get the pressure
        PRINTF("pretending to read a pressure value\r\n");
      }
    }

    PROCESS_END();
}
