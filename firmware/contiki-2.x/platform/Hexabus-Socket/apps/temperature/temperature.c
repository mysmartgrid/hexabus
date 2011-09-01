/*
 * temperature.c
 *
 *  Created on: 12.08.2011
 *      Author: Mathias Dalheimer
 */

#include "temperature.h"
#include <avr/io.h>
#include <util/delay.h>
#include "sys/clock.h"
#include "contiki.h"
#include "dev/leds.h"
#include <avr/eeprom.h>
#include "eeprom_variables.h"
#include "dev/leds.h"
#include "debug.h"
#include "tempsensors.h"

//local variables
static float temperature_value=0.0;

void
temperature_init(void)
{
  char buffer[10];
  PRINTF("-- Temperature: INIT\r\n");
  initTempSensors();
  loopTempSensors();
  temperature_value=getTemperatureFloat();
  dtostrf(temperature_value, 9, 4, &buffer);
  PRINTF("Current temp: %s deg C\r\n", buffer);
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
  return temperature_value;
}


