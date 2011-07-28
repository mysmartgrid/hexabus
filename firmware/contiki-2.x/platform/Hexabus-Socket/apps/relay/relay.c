/*
 * relay.c
 *
 *  Created on: 20.04.2011
 *      Author: Günter Hildebrandt
 */

#include "relay.h"
#include <stdbool.h>
#include <util/delay.h>
#include "dev/leds.h"
#include <avr/eeprom.h>
#include "eeprom_variables.h"


#define PRINTF(...) printf(__VA_ARGS__)
/** \brief This is a file internal variable that contains the default state of the relay.
 *
 */
static volatile bool relay_default_state;

/** \brief This is a is a file internal variable that contains the current relay state.
 *
 **/
static volatile bool relay_state;

void relay_leds(void)
{
	if (relay_state) {
	      leds_on(LEDS_RED);
	      leds_off(LEDS_GREEN);
	    }else {
		  leds_on(LEDS_GREEN);
		  leds_off(LEDS_RED);
	    }
}

bool
relay_get_state(void)
{
  return relay_state;
}

void
relay_toggle(void)
{
  if (relay_state)
    {
      relay_off();
    }
  else
    {
      relay_on();
    }
}

void
relay_on(void)
{
  if (!relay_state)
    {
	  ENABLE_RELAY_PWM();
	  //set PWM to 100% duty cycle
      SET_RELAY_PWM(RELAY_PWM_START);
#if RELAY_POWER_SAVING
      //sleep RELAY_OPERATE_TIME
      _delay_ms(RELAY_OPERATE_TIME);
      //set PWM to 50% duty cycle
      SET_RELAY_PWM(RELAY_PWM_HOLD);
#endif
      relay_state = 1;
      relay_leds();
      metering_reset();
    }
}

void
relay_off(void)
{
  DISABLE_RELAY_PWM();
  SET_RELAY_PWM(0x00);
  relay_state = 0;
  relay_leds();
  metering_reset();
}

void
relay_default(void)
{
  if (relay_default_state)
    {
      relay_on();
    }
  else
    {
      relay_off();
    }
}

void
set_relay_default(bool d_value)
{
  if (relay_default_state != d_value)
    {
      eeprom_write_byte((void*) EE_RELAY_DEFAULT, (unsigned char)d_value);
      relay_default_state = d_value;
    }
}

void
relay_init(void)
{
  /* Load reference values from EEPROM */
  relay_default_state = (bool) eeprom_read_byte((void*) EE_RELAY_DEFAULT);

  /*PWM Specific Initialization.*/
  SET_RELAY_TCCRxA();
  SET_RELAY_TCCRxB();

  //relay is off at init
  relay_state = 0;

  //set default state according to eeprom value
  relay_default();
}

