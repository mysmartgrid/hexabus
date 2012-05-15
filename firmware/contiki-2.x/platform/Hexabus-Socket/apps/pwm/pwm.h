/*
 * Copyright (c) 2012, Fraunhofer ITWM
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
 * Author: 	Dominik Keller <dominik.keller@itwm.fraunhofer.de>
 *
 * @(#)$$
 */

#ifndef PWM_H_
#define PWM_H_

#include <stdbool.h>
#include "process.h"

PROCESS_NAME(pwm_process);

// The HEXABUS-Socket hardware uses Timer/Counter0 
// if a different Timer/Counter is used, change number 0 in TCCR0A, TCR0B, OCR0A to the used timer/counter
#define SET_PWM_TCCRxA( ) ( TCCR0A = ( 0x00 | ( 1 << WGM01 ) | ( 1 << WGM00 ))) // WGM01 & WGM00: use fast PWM
#define SET_PWM_TCCRxB( ) ( TCCR0B = ( 0x00 | ( 1 << CS01 ) | ( 1 << CS00 ))) // CS00-CS02: Clock Select (F_CPU/64)


#if defined(__AVR_ATmega1284P__)
#define PWM_PORT	PORTB3
#elif defined(__AVR_ATmega2561__)
#define PWM_PORT	PORTB7
#else
#error Hardware not defined!
#endif

#define SET_PWM(PWM_MODE) (OCR0A = PWM_MODE)
#define ENABLE_PWM()	TCCR0A |= ( 1 << COM0A1 ); DDRB |= (1 << PWM_PORT) //COM0A1: clear OC0A on Compare Match, set OC0A at BOTTOM
#define DISABLE_PWM()	TCCR0A &= ~( 1 << COM0A1 ); DDRB &= ~(1 << PWM_PORT)

#define PWM_UPPER_LIMIT		0xFF //off->on PWM
#define PWM_LOWER_LIMIT		0x00 //hold on PWM (0x7F: 50%)

#endif /* PWM_H_ */
