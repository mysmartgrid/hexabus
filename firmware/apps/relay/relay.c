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
 * Author: 	G�nter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *
 * @(#)$$
 */

#include "relay.h"
#include <stdbool.h>
#include <util/delay.h>
#include "dev/leds.h"
#include "hexabus_config.h"
#include "metering.h"
#include "endpoints.h"
#include "endpoint_registry.h"
#include "nvm.h"


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
#if ! METERING_ENERGY_PERSISTENT
  if (!relay_state)
    {
#if RELAY_POWER_SAVING
	  ENABLE_RELAY_PWM();
	  //set PWM to 100% duty cycle
      SET_RELAY_PWM(RELAY_PWM_START);
      //sleep RELAY_OPERATE_TIME
      _delay_ms(RELAY_OPERATE_TIME);
      //set PWM to 50% duty cycle
      SET_RELAY_PWM(RELAY_PWM_HOLD);
#else
      PORTB |= (1 << PB3);
#endif
      relay_state = 1;
      relay_leds();
      metering_reset();
    }
#endif
}

void
relay_off(void)
{
#if ! METERING_ENERGY_PERSISTENT
#if RELAY_POWER_SAVING
  DISABLE_RELAY_PWM();
  SET_RELAY_PWM(0x00);
#else
  PORTB &= ~(1 << PB3);
#endif

  relay_state = 0;
  relay_leds();
  metering_reset();
#endif
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

void set_relay_default(bool d_value)
{
	if (relay_default_state != d_value) {
		nvm_write_u8(relay_default, d_value);
		relay_default_state = d_value;
	}
}

static enum hxb_error_code read(struct hxb_value* value)
{
	value->v_bool = relay_get_state() == 0 ? HXB_TRUE : HXB_FALSE;
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write(const struct hxb_envelope* env)
{
	if (env->value.v_bool == HXB_TRUE) {
		relay_off();  // Note that the relay is connected in normally-closed position, so relay_off turns the power on and vice-versa
	} else {
		relay_on();
	}
	return HXB_ERR_SUCCESS;
}

static const char ep_name[] PROGMEM = "Main Switch";
ENDPOINT_DESCRIPTOR endpoint_relay = {
	.datatype = HXB_DTYPE_BOOL,
	.eid = EP_POWER_SWITCH,
	.name = ep_name,
	.read = read,
	.write = write
};

void relay_init(void)
{
#if RELAY_ENABLE
#if ! METERING_ENERGY_PERSISTENT
	/* Load reference values from EEPROM */
	relay_default_state = nvm_read_u8(relay_default);

	/*PWM Specific Initialization.*/
#if RELAY_POWER_SAVING
	SET_RELAY_TCCRxA();
	SET_RELAY_TCCRxB();
#else
	DDRB |= (1 << PB3);
#endif

	//relay is off at init
	relay_state = 0;

	//set default state according to eeprom value
	relay_default();
#endif
	ENDPOINT_REGISTER(endpoint_relay);
#endif
}
