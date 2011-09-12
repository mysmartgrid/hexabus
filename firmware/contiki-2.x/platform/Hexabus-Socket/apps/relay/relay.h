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

#ifndef RELAY_H_
#define RELAY_H_

#include <stdbool.h>

#define RELAY_POWER_SAVING 0 //enable/disable PWM on relay


// The HEXABUS-Socket hardware uses Timer/Counter0 to switch the relay
// if a different Timer/Counter is used, change number 0 in TCCR0A, TCR0B, OCR0A to the used timer/counter
#define SET_RELAY_TCCRxA( ) ( TCCR0A = ( 0x00 | ( 1 << WGM01 ) | ( 1 << WGM00 ))) // WGM01 & WGM00: use fast PWM
#define SET_RELAY_TCCRxB( ) ( TCCR0B = ( 0x00 | ( 1 << CS01 ) | ( 1 << CS00 ))) // CS00-CS02: Clock Select (F_CPU/64)


#if defined(__AVR_ATmega1284P__)
#define RELAY_PWM_PORT	PORTB3
#elif defined(__AVR_ATmega2561__)
#define RELAY_PWM_PORT	PORTB7
#else
#error Hardware not defined!
#endif

#define SET_RELAY_PWM(PWM_MODE) (OCR0A = PWM_MODE)
#define ENABLE_RELAY_PWM()	TCCR0A |= ( 1 << COM0A1 ); DDRB |= (1 << RELAY_PWM_PORT) //COM0A1: clear OC0A on Compare Match, set OC0A at BOTTOM
#define DISABLE_RELAY_PWM()	TCCR0A &= ~( 1 << COM0A1 ); DDRB &= ~(1 << RELAY_PWM_PORT)

#define RELAY_OPERATE_TIME        		500	 //time in ms which relay needs for switching from off->on
#define RELAY_PWM_START                 0xFF //off->on PWM
#define RELAY_PWM_HOLD                  0x7F //hold on PWM (0x7F: 50%)



/** \brief This function toggles the relay.
 *
 **/
void 		relay_toggle(void);

/** \brief This function switches the relay on.
 *
 **/
void		relay_on(void);

/** \brief This function switches the relay off.
 *
 **/
void		relay_off(void);

/** \brief This function sets the default relay state in EEPROM.
 *
 **/
void            set_relay_default(bool);

/** \brief This function switches the relay to the default state.
 *
 **/
void            relay_default(void);

/** \brief This function returns the current relay state.
 *
 **/
bool            relay_get_state(void);

/** \brief This function initializes the relay program.
 *
 **/
void            relay_init(void);

/** \brief This function sets the LEDs accordingly the relay state.
 *
 **/
void 			relay_leds(void);


#endif /* RELAY_H_ */
