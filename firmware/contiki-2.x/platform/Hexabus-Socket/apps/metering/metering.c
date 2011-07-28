/*
 * metering.c
 *
 *  Created on: 20.04.2011
 *      Author: Günter Hildebrandt
 */

#include "metering.h"
#include <avr/io.h>
#include <util/delay.h>
#include "sys/clock.h"
#include "contiki.h"
#include "dev/leds.h"
#include <avr/eeprom.h>
#include "eeprom_variables.h"
#include "dev/leds.h"
#include "relay.h"

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
static uint16_t metering_calibration_power;
static uint16_t metering_reference_value;
static uint8_t metering_calibration_state = 0;
static uint16_t metering_power;
static bool metering_calibration = false;
/*calculates the consumed energy on the basis of the counted pulses (num_pulses) in i given period (integration_time)
 * P = 247.4W/Hz * f_CF */
uint16_t
calc_power(uint16_t pulse_period)
{
  uint16_t P;
  P = metering_reference_value / pulse_period;

  return P;
}

void
metering_init(void)
{
  /* Load reference values from EEPROM */
  metering_reference_value = eeprom_read_word((void*) EE_METERING_REF );
  metering_calibration_power = eeprom_read_word((void*)EE_METERING_CAL_LOAD);

  SET_METERING_INT();

  metering_start();
}

void
metering_start(void)
{
  /*Reset variables used in file.*/
  metering_pulse_period = 0;
  metering_power = 0;
  clock_old = clock_time();

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
}

uint16_t
metering_get_power(void)
{
  return metering_power;
}

bool
metering_calibrate(void)
{
	unsigned char cal_flag = eeprom_read_byte((void*)EE_METERING_CAL_FLAG);
	if (cal_flag == 0xFF) {
		metering_calibration = true;
		leds_on(LEDS_ALL);
		return true;
	}
	else
		return false;
}

//interrupt service routine for the metering interrupt
ISR(METERING_VECT)
{
  //calibration
  if (metering_calibration == true)    {
	  //do 10 measurements
      if (metering_calibration_state < 10)
        {
          if (metering_calibration_state == 0)
            {
              //reset Timer
              metering_pulse_period = clock_time();
            }
          metering_calibration_state++;
        }
      else {
		  //get mean pulse period over 10 cycles
		  if (clock_time() > metering_calibration_state++)
				metering_pulse_period = clock_time() - metering_pulse_period;
		  else //overflow of clock_time()
			  metering_pulse_period = 0xFFFF - metering_pulse_period + clock_time() + 1;

		  metering_pulse_period /= 10;

		  metering_reference_value = metering_pulse_period * metering_calibration_power;

		  //store calibration in EEPROM
		  eeprom_write_word((void*) EE_METERING_REF, metering_reference_value);

		  //lock calibration by setting flag in eeprom
		  eeprom_write_byte((void*) EE_METERING_CAL_FLAG, 0x00);

		  metering_calibration_state = 0;
		  metering_calibration = false;
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
	  	 metering_pulse_period = 0xFFFF - clock_old + clock_time() + 1;

	  clock_old = clock_time();
      //calculate and set electrical power
      metering_power = calc_power(metering_pulse_period);
    }
}



