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
#ifndef METERING_H_
#define METERING_H_

#include <stdint.h>
#include <stdbool.h>

/*if no interrupt happens in this time then the old measurement out of date and therefore discarded and set to 0 Watt
by reducing this value the accuracy for high power consumptions could be increased but the minimum measurable power consumption increases
for 80s: P_min = 3W */
#define OUT_OF_DATE_TIME   240

#define METERING_VECT     INT1_vect //Interrupt for the Metering


#define SET_METERING_INT( ) ( EICRA |= ((1 << ISC11) | (1 << ISC10)) ) // rising edge on INT1 generates interrupt

#define ENABLE_METERING_INTERRUPT( )       ( EIMSK |= ( 1 << INT1 ) )
#define DISABLE_METERING_INTERRUPT( )       ( EIMSK &= ~( 1 << INT1 ) )

#define METERING_POWERDOWN_DETECT_PIN PB3
#define METERING_ANALOG_COMPARATOR_REF_PIN PB2
#define ENABLE_POWERDOWN_INTERRUPT( )     ( ACSR = (0 << ACD) | (1 << ACBG) | (0 << ACO) | (1 << ACI) | (0 << ACIC) | (1 << ACIE) | (1 << ACIS1) | (1 << ACIS0) )
/*  ACSR = // setup analog comparator interrupt:
    (0 << ACD) | // Comparator on (Analog Comparator Disable := 0)
    (1 << ACBG) | // internal bandgap ref. voltage (1.23V) to AIN0
    (0 << ACO) | // Comparator output disable
    (1 << ACI) | // comparator interrupt flag
    (0 << ACIC) | // input capture off
    (1 << ACIE) | // comparator interrupt enable
    (1 << ACIS1) | (1 << ACIS0); // Interrupt on RISING edge (since the supply voltage is connected to negative input and Vbg is connected to positive input, rising edge means dropping supply voltage. */

/** \brief This function returns the measured electrical power in Watt.
 *
 **/
uint16_t   metering_get_power(void);
void     metering_init(void);
void    metering_start(void);
void    metering_stop(void);
void    metering_reset(void);
bool    metering_calibrate(void);
void    metering_calibration_stop(void);
void    metering_set_s0_calibration(uint16_t value);

#if METERING_ENERGY
float metering_get_energy(void);
float metering_get_energy_total(void);
float metering_reset_energy(void);
#endif // METERING_ENERGY

#endif /* METERING_H_ */

