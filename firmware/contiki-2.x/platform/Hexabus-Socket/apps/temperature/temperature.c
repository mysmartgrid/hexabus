/*
 * temperature.c
 *
 *  Created on: 12.08.2011
 *      Author: Mathias Dalheimer
 */

#include "temperature.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "sys/clock.h"
#include "contiki.h"
#include "dev/leds.h"
#include <avr/eeprom.h>
#include "eeprom_variables.h"
#include "dev/leds.h"
#include "debug.h"
#include "tempsensors.h"

static struct etimer temperature_periodic_timer;

//local variables
static float temperature_value=0.0;
static char temperature_string_buffer[10];

PROCESS(temperature_process, "HEXABUS Socket temperature Process");
AUTOSTART_PROCESSES(&temperature_process);

/*---------------------------------------------------------------------------*/
static void
pollhandler(void) {
	PRINTF("----Socket_temperature_handler: Process polled\r\n");
}

static void
exithandler(void) {
	PRINTF("----Socket_temperature_handler: Process exits.\r\n");
}
/*---------------------------------------------------------------------------*/

void _update_temp_string(void) {
  dtostrf(temperature_value, 9, 4, &temperature_string_buffer);
}

void
temperature_init(void)
{
  PRINTF("-- Temperature: INIT\r\n");
  initTempSensors();
  loopTempSensors();
  temperature_value=getTemperatureFloat();
  _update_temp_string();
  // PRINTF("Current temp: %s deg C\r\n", temperature_string_buffer);
}

void
temperature_start(void)
{
  PRINTF("-- Temperature: START\r\n");
}

void
temperature_stop(void)
{
  PRINTF("-- Temperature: STOP\r\n");
}

void
temperature_reset(void)
{
  PRINTF("-- Temperature: RESET\r\n");
}

float
temperature_get(void)
{
  PRINTF("-- Temperature: Get value\r\n");
  loopTempSensors();
  temperature_value=getTemperatureFloat();
  _update_temp_string();
  PRINTF("Current temp: %s deg C\r\n", temperature_string_buffer);
  return temperature_value;
}

char*
temperature_as_string(void)
{
  PRINTF("-- Temperature: Get string value\r\n");
  return &temperature_string_buffer;
}


/*---------------------------------------------------------------------------*/

PROCESS_THREAD(temperature_process, ev, data) {
	PROCESS_POLLHANDLER(pollhandler());
	PROCESS_EXITHANDLER(exithandler());

	// see: http://senstools.gforge.inria.fr/doku.php?id=contiki:examples
	PROCESS_BEGIN();
	PRINTF("temperature: process startup.\r\n");
	PROCESS_PAUSE();

	// wait 3 seconds
	etimer_set(&temperature_periodic_timer, CLOCK_CONF_SECOND*3);
	// wait until the timer has expired
	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

	// set the timer to 10 sec for use in the loop
	etimer_set(&temperature_periodic_timer, 10*CLOCK_SECOND);

	//everytime the timer event appears, get the temperature and reset the timer
	
	while(1){
		PROCESS_YIELD();
		if (etimer_expired(&temperature_periodic_timer)) {
			etimer_reset(&temperature_periodic_timer);
			temperature_get();
		}
	}
	PROCESS_END();
}
