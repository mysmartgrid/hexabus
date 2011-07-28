/*
 * Copyright (c) 2008, Technical University of Munich
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
 * This file is part of the Contiki operating system.
 *
 * @(#)$$
 */

/**
 * \addtogroup usbstick
 *
 */

#ifndef __CONTIKI_HEXABUS_H__
#define __CONTIKI_HEXABUS_H__

#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"

//GH: 18.04.2011 LEDs should not be toggled by Contiki
/*
#define Leds_init()                 leds_arch_init()
#define Leds_on()                   leds_on(LEDS_ALL)
#define Leds_off()                  leds_off(LEDS_ALL)
#define Led0_on()                   leds_on(LEDS_RED)
#define Led1_on()                   leds_on(LEDS_YELLOW)
#define Led2_on()                   leds_on(LEDS_GREEN)
#define Led3_on()                   leds_on(LEDS_RED)
#define Led0_off()                  leds_off(LEDS_RED)
#define Led1_off()                  leds_off(LEDS_YELLOW)
#define Led2_off()                  leds_off(LEDS_GREEN)
#define Led3_off()                  leds_off(LEDS_RED)
#define Led0_toggle()               leds_toggle(LEDS_RED)
#define Led1_toggle()               leds_toggle(LEDS_YELLOW)
#define Led2_toggle()               leds_toggle(LEDS_GREEN)
#define Led3_toggle()               leds_toggle(LEDS_RED)
*/

#define Leds_init(...)
#define Leds_on(...)
#define Leds_off(...)
#define Led0_on(...)
#define Led1_on(...)
#define Led2_on(...)
#define Led3_on(...)
#define Led0_off(...)
#define Led1_off(...)
#define Led2_off(...)
#define Led3_off(...)
#define Led0_toggle(...)
#define Led1_toggle(...)
#define Led2_toggle(...)
#define Led3_toggle(...)


void init_lowlevel(void);
void init_net(void);


#endif /* #ifndef __CONTIKI_HEXABUS_H__ */
