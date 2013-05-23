/*
 * Copyright (c) 2012, Fraunhofer ITWM
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
 * Author: 	Dominik Keller <dominik.keller@itwm.fraunhofer.de>
 *
 * @(#)$$
 */

#include "pwm.h"
#include <stdbool.h>
#include <util/delay.h>
#include "dev/leds.h"
#include <avr/eeprom.h>
#include "contiki.h"
#include "syslog.h"

void pwm_start(void){
  ENABLE_PWM();
  syslog(LOG_DEBUG, "PWM enabled");
}


void pwm_init(void){
  /*PWM Specific Initialization.*/
  SET_PWM_TCCRxA();
  SET_PWM_TCCRxB();
  syslog(LOG_DEBUG, "PWM init'd");
}


static struct etimer pwm_periodic_timer;
uint32_t pwm_ratio=0;
uint8_t counter=0;
bool toggle=1; //1 == count up, 0 == count down;

PROCESS(pwm_process, "HEXABUS PWM Process");
AUTOSTART_PROCESSES(&pwm_process);

/*---------------------------------------------------------------------------*/
static void
pollhandler(void) {
  syslog(LOG_DEBUG, "----Socket_pwm_handler: Process polled");
}

static void
exithandler(void) {
  syslog(LOG_DEBUG, "----Socket_pwm_handler: Process exits.\r\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(pwm_process, ev, data) {
  PROCESS_POLLHANDLER(pollhandler());
  PROCESS_EXITHANDLER(exithandler());

  // see: http://senstools.gforge.inria.fr/doku.php?id=contiki:examples
  PROCESS_BEGIN();
  syslog(LOG_DEBUG, "pwm: process startup.");
  PROCESS_PAUSE();

  // set the timer to 1 sec for use in the loop
  etimer_set(&pwm_periodic_timer, 0.01*CLOCK_SECOND);
  syslog(LOG_DEBUG, "PWM starting up.");

  while(1){
    PROCESS_YIELD();
    if(etimer_expired(&pwm_periodic_timer)) {
      etimer_reset(&pwm_periodic_timer);

      pwm_ratio=counter;

      // do some math to compensate for the non linear behavior of the LED
      uint32_t pwm_ratio_mod=(pwm_ratio*pwm_ratio)/(255);
      pwm_ratio=(pwm_ratio_mod*pwm_ratio)/(255);
      SET_PWM(pwm_ratio);

      if(pwm_ratio>=PWM_UPPER_LIMIT)
        toggle=0; //count down;
      if(pwm_ratio<=PWM_LOWER_LIMIT)
        toggle=1; //count up;

      if(toggle)
        counter++;
      else
        counter--;
      syslog(LOG_DEBUG, "counter: %d, pwm_ratio: %d, toggle: %d\r\n", counter, pwm_ratio, toggle);
    }
  }
  PROCESS_END();
}
