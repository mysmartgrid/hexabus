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
 * @(#)$$
 */

/**
 * \file
 *         AVR specific definitions for the rs232 port.   
 *
 * \author
 *         Martin Krogmann <martin.krogmann@esk.fraunhofer.de
 */

#ifndef __RS232_ATXMEGAD__
#define __RS232_ATXMEGAD__
/******************************************************************************/
/***   Includes                                                               */
/******************************************************************************/
#include "contiki-conf.h"
#include <avr/io.h>

/******************************************************************************/
/***   RS232 ports                                                            */
/******************************************************************************/
#define RS232_PORT_0 0
#define RS232_PORT_1 1
#define RS232_PORT_2 1

// PIN number on PORT
#define XCK0 1
#define RXD0 2
#define TXD0 3

/******************************************************************************/
/***   Baud rates                                                             */
/******************************************************************************/
#if F_CPU == 16000000
/* Single speed operation (U2X = 0)*/
#define USART_BAUD_2400 416
#define USART_BAUD_4800 207
#define USART_BAUD_9600 103
#define USART_BAUD_14400 68
#define USART_BAUD_19200 51
#define USART_BAUD_28800 34
#define USART_BAUD_38400 25
#define USART_BAUD_57600 16
#define USART_BAUD_76800 12
#define USART_BAUD_115200 8
#define USART_BAUD_230400 3
#define USART_BAUD_250000 3
#define USART_BAUD_500000 1
#define USART_BAUD_1000000 0

#else
#error "Please define the baud rates for your CPU clock \
 or set the rate in contiki-conf.h"
#endif


/******************************************************************************/
/***   Interrupt settings                                                     */
/******************************************************************************/
#define USART_INTERRUPT_RX_COMPLETE _BV (USART_RXCINTLVL0_bp)
#define USART_INTERRUPT_TX_COMPLETE _BV (USART_TXCINTLVL0_bp)
#define USART_INTERRUPT_DATA_REG_EMPTY _BV (USART_DREINTLVL0_bp)

/******************************************************************************/
/***   Receiver / transmitter                                                 */
/******************************************************************************/
#define USART_RECEIVER_ENABLE _BV (USART_RXEN_bp)
#define USART_TRANSMITTER_ENABLE _BV (USART_TXEN_bp)

/******************************************************************************/
/***   Mode select                                                            */
/******************************************************************************/
#define USART_MODE_ASYNC USART_CMODE_ASYNCHRONOUS_gc
#define USART_MODE_SYNC USART_CMODE_SYNCHRONOUS_gc

/******************************************************************************/
/***   Parity                                                                 */
/******************************************************************************/
#define USART_PARITY_NONE USART_PMODE_DISABLED_gc
#define USART_PARITY_EVEN USART_PMODE_EVEN_gc
#define USART_PARITY_ODD  USART_PMODE_ODD_gc

/******************************************************************************/
/***   Stop bits                                                              */
/******************************************************************************/
#define USART_STOP_BITS_1 0x00
#define USART_STOP_BITS_2 _BV (USART_SBMODE_bp)

/******************************************************************************/
/***   Character size                                                         */
/******************************************************************************/
#define USART_DATA_BITS_5 USART_CHSIZE_5BIT_gc
#define USART_DATA_BITS_6 USART_CHSIZE_6BIT_gc
#define USART_DATA_BITS_7 USART_CHSIZE_7BIT_gc
#define USART_DATA_BITS_8 USART_CHSIZE_8BIT_gc
#define USART_DATA_BITS_9 USART_CHSIZE_9BIT_gc

/******************************************************************************/
/***   Clock polarity                                                         */
/******************************************************************************/
/* TODO: KM not implemented yet
#define USART_RISING_XCKN_EDGE 0x00
#define USART_FALLING_XCKN_EDGE _BV (UCPOL)
*/

#endif /* #ifndef __RS232_ATXMEGAD__ */
