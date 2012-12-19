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
#include "hexabus_config.h"

#if TEMPERATURE_SENSOR == 1
#include "humidity.h"
#elif TEMPERATURE_SENSOR == 2
#include "pressure.h"
#endif

static struct etimer temperature_periodic_timer;

//local variables
static float temperature_value;
static char temperature_string_buffer[10];

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
#if TEMPERATURE_SENSOR == 0
  temperature_value=0.0;
  PRINTF("-- Temperature: INIT\r\n");
  initTempSensors();
  loopTempSensors();
  temperature_value=getTemperatureFloat();
  _update_temp_string();
  // PRINTF("Current temp: %s deg C\r\n", temperature_string_buffer);
#endif
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
  //getTemperatureFloat(); //TODO: Fixes really strange linker error, somebody needs to look into this. (loopTempSensors, getTemperatureFloat or read_pressure_temp must be executed to be able to compile. Possible bug with avr-binutils)
  //TODO: Now fixed by adding -lc -lm to linker flags, needs further testing
#if TEMPERATURE_SENSOR == 0
  PRINTF("-- Temperature: Get value\r\n");
  loopTempSensors();
  temperature_value=getTemperatureFloat();
  _update_temp_string();
  PRINTF("Current temp: %s deg C\r\n", temperature_string_buffer);
  return temperature_value;
#elif TEMPERATURE_SENSOR == 1
  return read_humidity_temp();
#elif TEMPERATURE_SENSOR == 2
  return read_pressure_temp();
#else
  return -1;
#endif
}

char*
temperature_as_string(void)
{
  PRINTF("-- Temperature: Get string value\r\n");
  return &temperature_string_buffer;
}
