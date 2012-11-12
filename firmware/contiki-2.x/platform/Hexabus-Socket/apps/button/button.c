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
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "dev/leds.h"
#include "contiki.h"

#include "hexabus_config.h"
#include <endpoints.h>

#include "eeprom_variables.h"
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "dev/watchdog.h"
#include <avr/interrupt.h>

#include "metering.h"
#include "relay.h"
#include "provisioning.h"


#if BUTTON_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if BUTTON_HAS_EID
  int button_pushed = 0;

  uint8_t button_get_pushed()
  {
    return button_pushed;
  }
#endif

PROCESS(button_pressed_process, "Check for pressed button process");
AUTOSTART_PROCESSES(&button_pressed_process);

PROCESS_THREAD(button_pressed_process, ev, data)
{
	PROCESS_EXITHANDLER(goto exit);

	static struct etimer button_timer;
	static unsigned char double_click;
	static clock_time_t time;
	PROCESS_BEGIN();

	while (1)
	{
		/*button is pressed when BUTTON_BIT is clear */
		if (bit_is_clear(BUTTON_PIN, BUTTON_BIT))
		{
			etimer_set(&button_timer, CLOCK_SECOND * DEBOUNCE_TIME / 1000);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&button_timer));
			if (bit_is_clear(BUTTON_PIN, BUTTON_BIT)) { //if button is still pressed
				etimer_restart(&button_timer);
				time = clock_time();
				double_click = 0;
				/*check for button release within LONG_CLICK_TIME*/
				do {
					PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&button_timer));
					etimer_restart(&button_timer);
					/* led feedback for provisioning */
					if (clock_time() - time > CLOCK_SECOND * CLICK_TIME / 1000){
						leds_off(LEDS_GREEN);
						provisioning_leds();
					}
				} while ((clock_time() - time < CLOCK_SECOND * LONG_CLICK_TIME / 1000) && (bit_is_clear(BUTTON_PIN, BUTTON_BIT)));
#if BUTTON_DOUBLE_CLICK_ENABLED
				/*button was released, check for a second click within DOUBLE_CLICK_TIME*/
				if (clock_time() - time < CLOCK_SECOND * DOUBLE_CLICK_TIME / 1000) {
					do {
						PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&button_timer));
						etimer_restart(&button_timer);
						double_click = bit_is_clear(BUTTON_PIN, BUTTON_BIT);
					} while ((clock_time() - time < CLOCK_SECOND * DOUBLE_CLICK_TIME / 1000) && (!double_click));
				}
				/*check for click type*/
				if (double_click && (clock_time() - time < CLOCK_SECOND * DOUBLE_CLICK_TIME / 1000))
				{ //DOUBLE CLICK
				}
#endif
				if (!double_click && clock_time() - time < CLOCK_SECOND * CLICK_TIME / 1000)
				{ //SHORT SINGLE CLICK
#if BUTTON_HAS_EID
          button_pushed = 1;
          broadcast_value(EP_BUTTON);
          button_pushed = 0;
#endif
#if BUTTON_TOGGLES_RELAY
					//toggle relay
					relay_toggle();
#endif
				}
				else if (!double_click && clock_time() - time < CLOCK_SECOND * LONG_CLICK_TIME / 1000)
				{ //SINGLE CLICK (>2s)
					//provisioning
					provisioning_slave();
				}
				else if (!double_click && clock_time() - time >= CLOCK_SECOND * LONG_CLICK_TIME / 1000) {//LONG SINGLE CLICK
					if (!metering_calibrate()) //start bootloader if calibration flag is not set
					{
						// bootloader (set flag in EEPROM and reboot)
						eeprom_write_byte((uint8_t *)EE_BOOTLOADER_FLAG, 0x01);
						watchdog_reboot();
					}
					else { //wait thus no second miss click can occur
						etimer_set(&button_timer, CLOCK_SECOND * 4 * PAUSE_TIME / 1000);
						PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&button_timer));
					}
				}
			}
			etimer_set(&button_timer, CLOCK_SECOND * PAUSE_TIME / 1000); //pause for PAUSE_TIME till next button event can occur
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&button_timer));
		}
		etimer_set(&button_timer, CLOCK_SECOND * DEBOUNCE_TIME / 1000);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&button_timer));
	}
	exit:
	;
	PROCESS_END();

}

