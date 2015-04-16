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
#include "contiki-conf.h"
#include "dev/leds.h"
#include <avr/io.h>

/* LED ports */

#define LEDS_PxDIR 			DDRC // port direction register
#define LEDS_PxOUT 			PORTC // port register


#define LEDS_CONF_RED    	(1<<PC0) //red led
#define LEDS_CONF_GREEN  	(1<<PC1) //green led


/* LEDs are enabled with 0 and disabled with 1. So we have to toggle the
 * values of LED_ON and LED_OFF in leds.h
 * */

void leds_arch_init(void)
{
  LEDS_PxDIR |= (LEDS_CONF_RED | LEDS_CONF_GREEN);
  LEDS_PxOUT |= (LEDS_CONF_RED | LEDS_CONF_GREEN);
}

unsigned char leds_arch_get(void)
{
  return ((LEDS_PxOUT & LEDS_CONF_RED) ? 0 : LEDS_RED)
    | ((LEDS_PxOUT & LEDS_CONF_GREEN) ? 0 : LEDS_GREEN);
}

void leds_arch_set(unsigned char leds)
{
  LEDS_PxOUT = (LEDS_PxOUT & ~(LEDS_CONF_RED|LEDS_CONF_GREEN))
    | ((leds & LEDS_RED) ? 0 : LEDS_CONF_RED)
    | ((leds & LEDS_GREEN) ? 0 : LEDS_CONF_GREEN);
}
