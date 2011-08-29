/*
 * button.c
 *
 *  Created on: 19.04.2011
 *      Author: Günter Hildebrandt
 */

#include "button.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "dev/leds.h"
#include "contiki.h"

#include "eeprom_variables.h"
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include "dev/watchdog.h"
#include <avr/interrupt.h>

#include "metering.h"
#include "relay.h"
#include "provisioning.h"


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
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
					PRINTF("button_process: button_state: %d, click time: %d\n",bit_is_clear(BUTTON_PIN, BUTTON_BIT), (clock_time() - time));
					/* led feedback for provisioning */
					if (clock_time() - time > CLOCK_SECOND * CLICK_TIME / 1000){
						leds_off(LEDS_GREEN);
						provisioning_leds();
					}
				} while ((clock_time() - time < CLOCK_SECOND * LONG_CLICK_TIME / 1000) && (bit_is_clear(BUTTON_PIN, BUTTON_BIT)));
#if DOUBLE_CLICK_ENABLED
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
				PRINTF("button_process: double click: %d, click time: %d\n",double_click, (clock_time() - time));
				if (!double_click && clock_time() - time < CLOCK_SECOND * CLICK_TIME / 1000)
				{ //SHORT SINGLE CLICK
					//toggle relay
					relay_toggle();
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
						eeprom_write_byte((void *)EE_BOOTLOADER_FLAG, 0x01);
						watchdog_reboot();
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

