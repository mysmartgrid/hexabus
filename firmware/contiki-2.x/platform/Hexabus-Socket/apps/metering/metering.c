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
 * Author:   Günter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *
 * @(#)$$
 */
#include "metering.h"
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "sys/clock.h"
#include "contiki.h"
#include "dev/leds.h"
#include <avr/eeprom.h>
#include "eeprom_variables.h"
#include "dev/leds.h"
#include "relay.h"
#include "hexabus_config.h"
#include "endpoints.h"
#include "endpoint_registry.h"
#include "value_broadcast.h"

/** \brief This is a file internal variable that contains the 16 MSB of the
 *         metering pulse period.
 *
 *         The metering pulse period (32-bit) is in METERING_TIMER_RESOLUTION. For the
 *         AVR microcontroller implementation this is solved by using a 16-bit
 *         timer (Timer1) with a clock frequency of 8 MHz and a prescaler of 1/1024.
 *         The metering_pulse_period is incremented when the 16-bit timer
 *         overflows, representing the 16 MSB. The timer value it self (TCNT1)
 *         is then the 16 LSB.
 *
 */

//local variables
static clock_time_t metering_pulse_period = 0;
static clock_time_t clock_old;
static uint16_t   metering_calibration_power;
static uint16_t   metering_reference_value;
static uint8_t     metering_calibration_state = 0;
static uint16_t   metering_power;
static bool     metering_calibration = false;
static struct ctimer metering_stop_timer;
#if METERING_IMMEDIATE_BROADCAST
static uint8_t ticks_until_broadcast = METERING_IMMEDIATE_BROADCAST_NUMBER_OF_TICKS;
static clock_time_t last_broadcast;
#endif
#if METERING_ENERGY
static uint32_t metering_pulses_total;
static uint32_t metering_pulses;
float metering_get_energy_total(void);
float metering_get_energy(void);
void metering_reset_energy(void);
#endif

/*calculates the consumed energy on the basis of the counted pulses (num_pulses) in i given period (integration_time)
 * P = 247.4W/Hz * f_CF */
uint16_t
calc_power(uint16_t pulse_period)
{
    uint32_t P;

#if S0_ENABLE
    P = ((uint32_t)metering_reference_value*10) / (uint32_t)pulse_period; //S0 measuring is scaled to fit calibration vlaue into eeprom.
#else
    P = metering_reference_value / pulse_period;
#endif

    return (uint16_t)P;
}

static enum hxb_error_code read_power(struct hxb_value* value)
{
	value->v_u32 = metering_get_power();
	return HXB_ERR_SUCCESS;
}

static const char ep_power_name[] PROGMEM = "Power Meter";
ENDPOINT_DESCRIPTOR endpoint_power = {
	.datatype = HXB_DTYPE_UINT32,
	.eid = EP_POWER_METER,
	.name = ep_power_name,
	.read = read_power,
	.write = 0
};

#if METERING_ENERGY
static enum hxb_error_code read_energy_total(struct hxb_value* value)
{
	value->v_float = metering_get_energy_total();
	return HXB_ERR_SUCCESS;
}

static const char ep_energy_total_name[] PROGMEM = "Energy Meter Total";
ENDPOINT_DESCRIPTOR endpoint_energy_total = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_ENERGY_METER_TOTAL,
	.name = ep_energy_total_name,
	.read = read_energy_total,
	.write = 0
};

static enum hxb_error_code read_energy(struct hxb_value* value)
{
	value->v_float = metering_get_energy();
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code write_energy(const struct hxb_envelope* value)
{
	metering_reset_energy();
	return HXB_ERR_SUCCESS;
}

static const char ep_energy_name[] PROGMEM = "Energy Meter";
ENDPOINT_DESCRIPTOR endpoint_energy = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_ENERGY_METER,
	.name = ep_energy_name,
	.read = read_energy,
	.write = write_energy
};
#endif

void
metering_init(void)
{
  /* Load reference values from EEPROM */
  metering_reference_value = eeprom_read_word((void*) EE_METERING_REF );
  metering_calibration_power = eeprom_read_word((void*)EE_METERING_CAL_LOAD);

#if METERING_ENERGY
  // reset energy metering value
  metering_pulses = metering_pulses_total = 0;
#if METERING_ENERGY_PERSISTENT
  DDRB &= ~(1 << METERING_POWERDOWN_DETECT_PIN); // Set PB3 as input -> Cut trace to relay, add voltage divider instead.
  DDRB &= ~(1 << METERING_ANALOG_COMPARATOR_REF_PIN); // Set PB2 as input
  PORTB &= ~(1 << METERING_POWERDOWN_DETECT_PIN); // no internal pullup
  PORTB &= ~(1 << METERING_ANALOG_COMPARATOR_REF_PIN); // no internal pullup
  ENABLE_POWERDOWN_INTERRUPT();
  sei(); // global interrupt enable

  eeprom_read((uint8_t*)EE_ENERGY_METERING_PULSES_TOTAL, (uint8_t*)&metering_pulses_total, sizeof(metering_pulses));
  eeprom_read((uint8_t*)EE_ENERGY_METERING_PULSES, (uint8_t*)&metering_pulses, sizeof(metering_pulses));
#endif // METERING_ENERGY_PERSISTENT
#endif // METERING_ENERGY

  SET_METERING_INT();

  metering_start();

#if METERING_POWER
	ENDPOINT_REGISTER(endpoint_power);
#endif
#if METERING_ENERGY
	ENDPOINT_REGISTER(endpoint_energy_total);
	ENDPOINT_REGISTER(endpoint_energy);
#endif
}

void
metering_start(void)
{
  /*Reset variables used in file.*/
  metering_pulse_period = 0;
  metering_power = 0;
  clock_old = clock_time();

#if METERING_IMMEDIATE_BROADCAST
  last_broadcast = clock_time();
#endif

  ENABLE_METERING_INTERRUPT(); /* Enable external interrupt. */
}

void
metering_stop(void)
{
  DISABLE_METERING_INTERRUPT(); /* Disable external capture interrupt. */
  metering_reset();
}

void
metering_reset(void)
{
  metering_power = 0;
	broadcast_value(EP_POWER_METER);
#if METERING_ENERGY
	broadcast_value(EP_ENERGY_METER_TOTAL);
#endif
}

#if METERING_ENERGY
float metering_get_energy(void)
{
  // metering_reference_value are (wattseconds * CLOCK_SECOND) per pulse
  // then divide by 3600 * 1000 to geht kilowatthours
  return (float)(metering_pulses * (metering_reference_value / CLOCK_SECOND)) / 3600000;
}

float metering_get_energy_total(void)
{
  // metering_reference_value are (wattseconds * CLOCK_SECOND) per pulse
  // then divide by 3600 * 1000 to geht kilowatthours
  return (float)(metering_pulses_total * (metering_reference_value / CLOCK_SECOND)) / 3600000;
}

void metering_reset_energy(void)
{
  metering_pulses = 0;
}
#endif

uint16_t
metering_get_power(void)
{
  uint16_t tmp;
  /*check whether measurement is up to date */
  if (clock_time() > clock_old)
    tmp = (clock_time() - clock_old);
  else
    tmp = (0xFFFF - clock_old + clock_time() + 1);

  if (tmp > OUT_OF_DATE_TIME * CLOCK_SECOND)
    metering_power = 0;

#if S0_ENABLE
  else if (metering_power != 0 && tmp > 2 * (((uint32_t)metering_reference_value*10) / (uint32_t)metering_power)) //S0 calibration is scaled
#else
  else if (metering_power != 0 && tmp > 2 * (metering_reference_value / metering_power))
#endif
      metering_power = calc_power(tmp);

  return metering_power;
}

/* sets calibration value for s0 meters */
void
metering_set_s0_calibration(uint16_t value) {
    metering_stop();
    eeprom_write_word((uint16_t*) EE_METERING_REF, ((3600000*CLOCK_SECOND)/(value*10))); 
    metering_init();
    metering_start();
}

/* starts the metering calibration if the calibration flag is 0xFF else returns 0*/
bool
metering_calibrate(void)
{
  unsigned char cal_flag = eeprom_read_byte((void*)EE_METERING_CAL_FLAG);
  if (cal_flag == 0xFF && !metering_calibration) {
    metering_calibration = true;
		metering_calibration_state = 0;
		relay_off();
    leds_on(LEDS_ALL);
    //stop calibration if there was no pulse in 60 seconds
    ctimer_set(&metering_stop_timer, CLOCK_SECOND*60,(void *)(void *) metering_calibration_stop,NULL);
  }
	return metering_calibration;
}

/* if socket is not equipped with metering then calibration should stop automatically after some time */
void
metering_calibration_stop(void)
{
  //store calibration in EEPROM
  eeprom_write_word((uint16_t*) EE_METERING_REF, 0);

  //lock calibration by setting flag in eeprom
  eeprom_write_byte((uint8_t*) EE_METERING_CAL_FLAG, 0x00);

  metering_calibration_state = 0;
  metering_calibration = false;
  relay_leds();
  ctimer_stop(&metering_stop_timer);
}

//interrupt service routine for the metering interrupt
ISR(METERING_VECT)
{
   //calibration
   if (metering_calibration == true)
   {
     //do 10 measurements
     if (metering_calibration_state < 10)
     {
       if (metering_calibration_state == 0)
       {
         //get current time
         metering_pulse_period = clock_time();
         //stop callback timer
         ctimer_stop(&metering_stop_timer);
       }
       metering_calibration_state++;
     }
     else
     {
       //get mean pulse period over 10 cycles
       if (clock_time() > metering_pulse_period)
         metering_pulse_period = clock_time() - metering_pulse_period;
       else //overflow of clock_time()
         metering_pulse_period = (0xFFFF - metering_pulse_period) + clock_time() + 1;

       metering_pulse_period /= 10;

       metering_reference_value = metering_pulse_period * metering_calibration_power;

       //store calibration in EEPROM
       eeprom_write_word((uint16_t*) EE_METERING_REF, metering_reference_value);

       //lock calibration by setting flag in eeprom
       eeprom_write_byte((uint8_t*) EE_METERING_CAL_FLAG, 0x00);

       metering_calibration_state = 0;
       metering_calibration = false;
			 relay_on();
       relay_leds();
     }
   }

  //measurement
  else
  {
    //get pulse period
    if (clock_time() > clock_old)
      metering_pulse_period = clock_time() - clock_old;
    else //overflow of clock_time()
      metering_pulse_period = (0xFFFF - clock_old) + clock_time() + 1;

    clock_old = clock_time();
    //calculate and set electrical power
    metering_power = calc_power(metering_pulse_period);

#if METERING_IMMEDIATE_BROADCAST
    uint16_t broadcast_timeout;
    if(!--ticks_until_broadcast)
    {
      ticks_until_broadcast = METERING_IMMEDIATE_BROADCAST_NUMBER_OF_TICKS;
      // check overflow
      if(clock_time() > last_broadcast)
        broadcast_timeout = (clock_time() - last_broadcast);
      else
        broadcast_timeout = (0xFFFF - last_broadcast) + clock_time() + 1;

      if(broadcast_timeout > METERING_IMMEDIATE_BROADCAST_MINIMUM_TIMEOUT * CLOCK_SECOND)
      {
        // the last argument is a void* that can be used for anything. We use it to tell value_broadcast our EID.
        // process_post(&value_broadcast_process, immediate_broadcast_event, (void*)2);
        last_broadcast = clock_time();

				static uint32_t last_power = 0xffffffff;
				if (last_power != metering_power) {
					broadcast_value(EP_POWER_METER);
					last_power = metering_power;
				}
      }
    }
#endif // METERING_IMMEDIATE_BROADCAST
#if METERING_ENERGY
    metering_pulses++;
    metering_pulses_total++;
#endif
  }
}

#if METERING_ENERGY_PERSISTENT
ISR(ANALOG_COMP_vect)
{
  if(metering_calibration == false && clock_time() > CLOCK_SECOND) // Don't do anything if calibration is enabled; don't do anything within one second after bootup. TODO: can the clock overflow?
  {
    eeprom_write((uint8_t*)EE_ENERGY_METERING_PULSES, (uint8_t*)&metering_pulses, sizeof(metering_pulses)); // write number of pulses to eeprom
    eeprom_write((uint8_t*)EE_ENERGY_METERING_PULSES_TOTAL, (uint8_t*)&metering_pulses_total, sizeof(metering_pulses_total));
    while(1); // wait until power fails completely or watchdog timer resets us if power comes back
  }
}
#endif // METERING_ENERGY_PERSISTENT

