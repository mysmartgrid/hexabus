/*
 * Copyright (c) 2007, Swedish Institute of Computer Science
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
 * $Id: rf212bb.c,v 1.2 2010/02/12 16:41:02 dak664 Exp $
 */

/**
 *  \brief This module contains radio driver code for the Atmel
 *  AT86RF212. It is modified to use the contiki core MAC layer.
 *
 *  \author Günter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *  		Blake Leverett <bleverett@gmail.com>
 *          Mike Vidales <mavida404@gmail.com>
 *          Eric Gnoske <egnoske@gmail.com>
 *          David Kopf <dak664@embarqmail.com>
 *
 */

/**  \addtogroup wireless
 * @{
 */

/**
 *  \defgroup radiorf212 RF212 interface
 * @{
 */
/**
 *  \file
 *  This file contains "barebones" radio driver code for use with the
 *  contiki core MAC layer.
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "contiki.h"

//#if defined(__AVR__)
#include <avr/io.h>
#include <util/delay_basic.h>
#define delay_us( us )   ( _delay_loop_2(1+(us*F_CPU)/4000000UL) )

//#include <util/delay.h>
#include <avr/pgmspace.h>
//#elif defined(__MSP430__)
//#include <io.h>
//#endif

#include "dev/leds.h"
#include "dev/spi.h"
#include "rf212bb.h"
#include "hal.h"
#include "net/mac/frame802154.h"
#include "radio.h"
#include <dev/watchdog.h>

#include "net/packetbuf.h"
#include "net/rime/rimestats.h"
#include "net/netstack.h"

//#include "net/rime/packetbuf.h"
//#include "net/rime/rimestats.h"

#include "sys/timetable.h"

#define WITH_SEND_CCA 0

/* See clock.c and httpd-cgi.c for RADIOSTATS code */

uint8_t RF212_radio_on;
#define RADIOSTATS 0
#if RADIOSTATS
uint8_t RF212_rsigsi;
uint16_t RF212_sendpackets,RF212_receivepackets,RF212_sendfail,RF212_receivefail;
#endif

/* Show if we are in bootloader mode */
extern volatile int bootloader_mode;

/* Show if we are in bootloader mode */
uint8_t promiscuous_mode;

#if RF212_CONF_TIMESTAMPS
#include "net/rime/timesynch.h"
#define TIMESTAMP_LEN 3
#else /* RF212_CONF_TIMESTAMPS */
#define TIMESTAMP_LEN 0
#endif /* RF212_CONF_TIMESTAMPS */
#define FOOTER_LEN 0
//#define FOOTER_LEN 0

#ifndef RF212_CONF_AUTORETRIES
#define RF212_CONF_AUTORETRIES 2
#endif /* RF230_CONF_AUTOACK */

#ifndef RF212_CONF_CHECKSUM
#define RF212_CONF_CHECKSUM 0
#endif /* RF212_CONF_CHECKSUM */

#if RF212_CONF_CHECKSUM
#include "lib/crc16.h"
#define CHECKSUM_LEN 2
#else
#define CHECKSUM_LEN 2
#endif /* RF212_CONF_CHECKSUM */

#define AUX_LEN (CHECKSUM_LEN + TIMESTAMP_LEN + FOOTER_LEN)

struct timestamp
{
	uint16_t time;
	uint8_t authority_level;
};

#define FOOTER1_CRC_OK      0x80
#define FOOTER1_CORRELATION 0x7f

#define DEBUG 0
#if DEBUG
#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTF(...) do {} while (0)
#endif

/* XXX hack: these will be made as Chameleon packet attributes */
rtimer_clock_t rf212_time_of_arrival, rf212_time_of_departure;

int rf212_authority_level_of_sender;

#if RF212_CONF_TIMESTAMPS
static rtimer_clock_t setup_time_for_transmission;
static unsigned long total_time_for_transmission, total_transmission_len;
static int num_transmissions;
#endif /* RF212_CONF_TIMESTAMPS */

#if DEBUGFLOWSIZE
uint8_t debugflowsize,debugflow[DEBUGFLOWSIZE];
#define DEBUGFLOW(c) if (debugflowsize<(DEBUGFLOWSIZE-1)) debugflow[debugflowsize++]=c
#else
#define DEBUGFLOW(c)
#endif

static uint8_t volatile pending;

/* RF212 hardware delay times, from datasheet */
typedef enum
{
	TIME_TO_ENTER_P_ON = 330, /**<  Transition time from VCC is applied to P_ON. */
	TIME_P_ON_TO_TRX_OFF = 330, /**<  Transition time from P_ON to TRX_OFF. */
	TIME_SLEEP_TO_TRX_OFF = 380, /**<  Transition time from SLEEP to TRX_OFF. */
	TIME_RESET = 1, /**<  Time to hold the RST pin low during reset		see p. 151 t_10=625ns*/
	TIME_ED_MEASUREMENT = 400, /**<  Time it takes to do a ED measurement. 		8 symbols at 20kbit/s BPSK */
	TIME_CCA = 400, /**<  Time it takes to do a CCA.  					8 symbols at 20kbit/s BPSK */
	TIME_PLL_LOCK = 370, /**<  Maximum time it should take for the PLL to lock. */
	TIME_FTN_TUNING = 25, /**<  Maximum time it should take to do the filter tuning.*/
	TIME_NOCLK_TO_WAKE = 6, /**<  Transition time from *_NOCLK to being awake. TODO gh: not in manual*/
	TIME_CMD_FORCE_TRX_OFF = 1, /**<  Time it takes to execute the FORCE_TRX_OFF command. */
	TIME_TRX_OFF_TO_PLL_ACTIVE = 200, /**<  Transition time from TRX_OFF to: RX_ON, PLL_ON, TX_ARET_ON and RX_AACK_ON. */
	TIME_STATE_TRANSITION_PLL_ACTIVE = 1,
/**<  Transition time from PLL active state to another. */
} radio_trx_timing_t;

/*---------------------------------------------------------------------------*/PROCESS(rf212_process, "RF212 receiver")
;
/*---------------------------------------------------------------------------*/

static int rf212_prepare(const void *data, unsigned short len);
static int rf212_transmit(unsigned short len);
int rf212_send(const void *data, unsigned short len);
int rf212_read(void *buf, unsigned short bufsize);
int rf212_on(void);
int rf212_off(void);

static int rf212_receiving_packet(void);
static int pending_packet(void);
static int rf212_cca(void);

const struct radio_driver rf212_driver =
{ rf212_init, rf212_prepare, rf212_transmit, rf212_send, rf212_read, rf212_cca,
		rf212_receiving_packet, pending_packet, rf212_on, rf212_off, };

uint8_t RF212_receive_on, RF212_sleeping;
static struct etimer rf212_calibration_timer;

static void (* receiver_callback)(const struct radio_driver *);

static void rf212_calibrate();

//signed char rf212_last_rssi;
//uint8_t rf212_last_correlation;
//static uint8_t rssi_val;
uint8_t rx_mode;
/* Radio stuff in network byte order. */
//static uint16_t pan_id;

//static int channel;


/*----------------------------------------------------------------------------*/
/** \brief  This function return the Radio Transceivers current state.
 *
 *  \retval     P_ON               When the external supply voltage (VDD) is
 *                                 first supplied to the transceiver IC, the
 *                                 system is in the P_ON (Poweron) mode.
 *  \retval     BUSY_RX            The radio transceiver is busy receiving a
 *                                 frame.
 *  \retval     BUSY_TX            The radio transceiver is busy transmitting a
 *                                 frame.
 *  \retval     RX_ON              The RX_ON mode enables the analog and digital
 *                                 receiver blocks and the PLL frequency
 *                                 synthesizer.
 *  \retval     TRX_OFF            In this mode, the SPI module and crystal
 *                                 oscillator are active.
 *  \retval     PLL_ON             Entering the PLL_ON mode from TRX_OFF will
 *                                 first enable the analog voltage regulator. The
 *                                 transceiver is ready to transmit a frame.
 *  \retval     BUSY_RX_AACK       The radio was in RX_AACK_ON mode and received
 *                                 the Start of Frame Delimiter (SFD). State
 *                                 transition to BUSY_RX_AACK is done if the SFD
 *                                 is valid.
 *  \retval     BUSY_TX_ARET       The radio transceiver is busy handling the
 *                                 auto retry mechanism.
 *  \retval     RX_AACK_ON         The auto acknowledge mode of the radio is
 *                                 enabled and it is waiting for an incomming
 *                                 frame.
 *  \retval     TX_ARET_ON         The auto retry mechanism is enabled and the
 *                                 radio transceiver is waiting for the user to
 *                                 send the TX_START command.
 *  \retval     RX_ON_NOCLK        The radio transceiver is listening for
 *                                 incomming frames, but the CLKM is disabled so
 *                                 that the controller could be sleeping.
 *                                 However, this is only true if the controller
 *                                 is run from the clock output of the radio.
 *  \retval     RX_AACK_ON_NOCLK   Same as the RX_ON_NOCLK state, but with the
 *                                 auto acknowledge module turned on.
 *  \retval     BUSY_RX_AACK_NOCLK Same as BUSY_RX_AACK, but the controller
 *                                 could be sleeping since the CLKM pin is
 *                                 disabled.
 *  \retval     STATE_TRANSITION   The radio transceiver's state machine is in
 *                                 transition between two states.
 */
uint8_t radio_get_trx_state(void)
{
	return hal_subregister_read(SR_TRX_STATUS);
}

/*----------------------------------------------------------------------------*/
/** \brief  This function checks if the radio transceiver is sleeping.
 *
 *  \retval     true    The radio transceiver is in SLEEP or one of the *_NOCLK
 *                      states.
 *  \retval     false   The radio transceiver is not sleeping.
 */bool radio_is_sleeping(void)
{
	bool sleeping = false;

	/* The radio transceiver will be at SLEEP or one of the *_NOCLK states only if */
	/* the SLP_TR pin is high. */
	if (hal_get_slptr() != 0)
	{
		sleeping = true;
	}

	return sleeping;
}

/*----------------------------------------------------------------------------*/
/** \brief  This function will reset the state machine (to TRX_OFF) from any of
 *          its states, except for the SLEEP state.
 */
void radio_reset_state_machine(void)
{
	hal_set_slptr_low();
	delay_us(TIME_NOCLK_TO_WAKE);
	hal_subregister_write(SR_TRX_CMD, CMD_FORCE_TRX_OFF);
	delay_us(TIME_CMD_FORCE_TRX_OFF);
}
/*----------------------------------------------------------------------------*/
/** \brief  This function will change the current state of the radio
 *          transceiver's internal state machine.
 *
 *  \param     new_state        Here is a list of possible states:
 *             - RX_ON        Requested transition to RX_ON state.
 *             - TRX_OFF      Requested transition to TRX_OFF state.
 *             - PLL_ON       Requested transition to PLL_ON state.
 *             - RX_AACK_ON   Requested transition to RX_AACK_ON state.
 *             - TX_ARET_ON   Requested transition to TX_ARET_ON state.
 *
 *  \retval    RADIO_SUCCESS          Requested state transition completed
 *                                  successfully.
 *  \retval    RADIO_INVALID_ARGUMENT Supplied function parameter out of bounds.
 *  \retval    RADIO_WRONG_STATE      Illegal state to do transition from.
 *  \retval    RADIO_BUSY_STATE       The radio transceiver is busy.
 *  \retval    RADIO_TIMED_OUT        The state transition could not be completed
 *                                  within resonable time.
 */
radio_status_t radio_set_trx_state(uint8_t new_state)
{
	uint8_t original_state;

	/*Check function paramter and current state of the radio transceiver.*/
	if (!((new_state == TRX_OFF) || (new_state == RX_ON) || (new_state
			== PLL_ON) || (new_state == RX_AACK_ON)
			|| (new_state == TX_ARET_ON)))
	{
		return RADIO_INVALID_ARGUMENT;
	}

	if (radio_is_sleeping() == true)
	{
		return RADIO_WRONG_STATE;
	}

	// Wait for radio to finish previous operation
	for (;;)
	{
		original_state = radio_get_trx_state();
		if (original_state != BUSY_TX_ARET && original_state != BUSY_RX_AACK
				&& original_state != BUSY_RX && original_state != BUSY_TX)
			break;
	}

	if (new_state == original_state)
	{
		return RADIO_SUCCESS;
	}

	/* At this point it is clear that the requested new_state is: */
	/* TRX_OFF, RX_ON, PLL_ON, RX_AACK_ON or TX_ARET_ON. */

	/* The radio transceiver can be in one of the following states: */
	/* TRX_OFF, RX_ON, PLL_ON, RX_AACK_ON, TX_ARET_ON. */
	if (new_state == TRX_OFF)
	{
		radio_reset_state_machine(); /* Go to TRX_OFF from any state. */
	}
	else
	{
		/* It is not allowed to go from RX_AACK_ON or TX_AACK_ON and directly to */
		/* TX_AACK_ON or RX_AACK_ON respectively. Need to go via RX_ON or PLL_ON. */
		if ((new_state == TX_ARET_ON) && (original_state == RX_AACK_ON))
		{
			/* First do intermediate state transition to PLL_ON, then to TX_ARET_ON. */
			/* The final state transition to TX_ARET_ON is handled after the if-else if. */
			hal_subregister_write(SR_TRX_CMD, PLL_ON);
			delay_us(TIME_STATE_TRANSITION_PLL_ACTIVE);
		}
		else if ((new_state == RX_AACK_ON) && (original_state == TX_ARET_ON))
		{
			/* First do intermediate state transition to PLL_ON, then to RX_AACK_ON. */
			/* The final state transition to RX_AACK_ON is handled after the if-else if. */
			hal_subregister_write(SR_TRX_CMD, PLL_ON);
			delay_us(TIME_STATE_TRANSITION_PLL_ACTIVE);
		}

		/* Any other state transition can be done directly. */
		hal_subregister_write(SR_TRX_CMD,new_state) ;

		/* When the PLL is active most states can be reached in 1us. However, from */
		/* TRX_OFF the PLL needs time to activate. */
		if (original_state == TRX_OFF)
		{
			delay_us(TIME_TRX_OFF_TO_PLL_ACTIVE);
		}
		else
		{
			delay_us(TIME_STATE_TRANSITION_PLL_ACTIVE);
		}
	} /*  end: if(new_state == TRX_OFF) ... */

	/*Verify state transition.*/
	radio_status_t set_state_status = RADIO_TIMED_OUT;

	if (radio_get_trx_state() == new_state)
	{
		set_state_status = RADIO_SUCCESS;
		/*  set rx_mode flag based on mode we're changing to */
		if (new_state == RX_ON || new_state == RX_AACK_ON)
		{
			rx_mode = true;
		}
		else
		{
			rx_mode = false;
		}
	}

	return set_state_status;
}

/*---------------------------------------------------------------------------*/
void rf212_waitidle(void)
{
	//	PRINTF("rf212_waitidle");
	uint8_t radio_state;

	for (;;)
	{
		radio_state = hal_subregister_read(SR_TRX_STATUS);
		watchdog_periodic();
		if (radio_state != BUSY_TX_ARET && radio_state != BUSY_RX_AACK
				&& radio_state != BUSY_RX && radio_state != BUSY_TX)
		{
			PRINTF("\n");
			break;
		}

		PRINTF(".");
	}
}

/*---------------------------------------------------------------------------*/
static uint8_t locked, lock_on, lock_off;

static void on(void)
{
	ENERGEST_ON(ENERGEST_TYPE_LISTEN);
	PRINTF("rf212 internal on\n");
	RF212_radio_on = 1;

	hal_set_slptr_low();
	//radio_is_waking=1;//can test this before tx instead of delaying
	delay_us(TIME_SLEEP_TO_TRX_OFF);
	delay_us(TIME_SLEEP_TO_TRX_OFF);//extra delay for now

	radio_set_trx_state(RX_AACK_ON);
	// flushrx();
}
static void off(void)
{
	PRINTF("rf212 internal off\n");
	RF212_radio_on = 0;

	/* Wait for transmission to end before turning radio off. */
	rf212_waitidle();

	/* Force the device into TRX_OFF. */
	radio_reset_state_machine();

	/* Sleep Radio */hal_set_slptr_high();

	ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
}
/*---------------------------------------------------------------------------*/
#define GET_LOCK() locked = 1
static void RELEASE_LOCK(void)
{
	if (lock_on)
	{
		on();
		lock_on = 0;
	}
	if (lock_off)
	{
		off();
		lock_off = 0;
	}
	locked = 0;
}

/*---------------------------------------------------------------------------*/
static int rf212_cca(void)
{
	int cca;
	int radio_was_off = 0;

	/* If the radio is locked by an underlying thread (because we are
	 being invoked through an interrupt), we preted that the coast is
	 clear (i.e., no packet is currently being transmitted by a
	 neighbor). */
	if (locked)
	{
		return 1;
	}

	if (!RF212_receive_on)
	{
		radio_was_off = 1;
		rf212_on();
	}

	DEBUGFLOW('c');
	/* CCA Mode Mode 1=Energy above threshold  2=Carrier sense only  3=Both 0=Either (RF231 only) */
	/* Use the current mode. Note triggering a manual CCA is not recommended in extended mode */
	//hal_subregister_write(SR_CCA_MODE,1);

	/* Start the CCA, wait till done, return result */
	hal_subregister_write(SR_CCA_REQUEST, 1);
	delay_us(TIME_CCA);
	//while ((hal_register_read(RG_TRX_STATUS) & 0x80) == 0 ) {continue;}
			while (!hal_subregister_read(SR_CCA_DONE))
			{	continue;}
			cca=hal_subregister_read(SR_CCA_STATUS);

			if(radio_was_off)
			{
				rf212_off();
			}
			return cca;
		}

	/*---------------------------------------------------------------------------*/
int rf212_receiving_packet(void)
{
	uint8_t radio_state;
	radio_state = hal_subregister_read(SR_TRX_STATUS);
	if ((radio_state == BUSY_RX) || (radio_state == BUSY_RX_AACK))
	{
		DEBUGFLOW('B');
		return 1;
	}
	else
	{
		return 0;
	}
}

/*---------------------------------------------------------------------------*/
static int pending_packet(void)
{
	if (pending)
		DEBUGFLOW('p');
	return pending;
}

/*---------------------------------------------------------------------------*/
int rf212_off(void)
{
	// PRINTF("rf212_off\n");
	/* Don't do anything if we are already turned off. */
	if (RF212_radio_on == 0)
	{
		return 1;
	}

	/* If we are called when the driver is locked, we indicate that the
	 radio should be turned off when the lock is unlocked. */
	if (locked)
	{
		lock_off = 1;
		return 1;
	}
	off();

	return 1;
}
/*---------------------------------------------------------------------------*/
int rf212_on(void)
{
	//PRINTF("rf212_on\n");
	if (RF212_radio_on)
	{
		return 1;
	}
	if (locked)
	{
		lock_on = 1;
		return 1;
	}
	on();

	return 1;
}
/*---------------------------------------------------------------------------*/
int rf212_get_channel(void)
{
	return hal_subregister_read(SR_CHANNEL);
	//	return channel;
}
/*---------------------------------------------------------------------------*/
void rf212_set_channel(int c)
{

	if (c > RF212_MAX_CHANNEL || c < RF212_MIN_CHANNEL)
	{
		PRINTF("rf212: invalid channel number %d", c);
		return;
	}

	/* Wait for any transmission to end. */
	rf212_waitidle();
	hal_subregister_write(SR_CHANNEL,c) ;

}
/*---------------------------------------------------------------------------*/
void rf212_set_pan_addr(uint16_t pan, uint16_t addr, uint8_t *ieee_addr)
{
	PRINTF("rf212: PAN=%x Short Addr=%x\n",pan,addr);

	uint8_t abyte;
	abyte = pan & 0xFF;
	hal_register_write(RG_PAN_ID_0, abyte);
	abyte = (pan >> 8 * 1) & 0xFF;
	hal_register_write(RG_PAN_ID_1, abyte);

	abyte = addr & 0xFF;
	hal_register_write(RG_SHORT_ADDR_0, abyte);
	abyte = (addr >> 8 * 1) & 0xFF;
	hal_register_write(RG_SHORT_ADDR_1, abyte);

	if (ieee_addr != NULL)
	{
		PRINTF("MAC=%x",*ieee_addr);
		hal_register_write(RG_IEEE_ADDR_7, *ieee_addr++);
		PRINTF(":%x",*ieee_addr);
		hal_register_write(RG_IEEE_ADDR_6, *ieee_addr++);
		PRINTF(":%x",*ieee_addr);
		hal_register_write(RG_IEEE_ADDR_5, *ieee_addr++);
		PRINTF(":%x",*ieee_addr);
		hal_register_write(RG_IEEE_ADDR_4, *ieee_addr++);
		PRINTF(":%x",*ieee_addr);
		hal_register_write(RG_IEEE_ADDR_3, *ieee_addr++);
		PRINTF(":%x",*ieee_addr);
		hal_register_write(RG_IEEE_ADDR_2, *ieee_addr++);
		PRINTF(":%x",*ieee_addr);
		hal_register_write(RG_IEEE_ADDR_1, *ieee_addr++);
		PRINTF(":%x",*ieee_addr);
		hal_register_write(RG_IEEE_ADDR_0, *ieee_addr);
		PRINTF("\n");
	}

}

/*---------------------------------------------------------------------------*/
/* Process to handle input packets
 * Receive interrupts cause this process to be polled
 * It calls the core MAC layer which calls rf212_read to get the packet
 */
#if 0
uint8_t rf212processflag;
#define RF212PROCESSFLAG(arg) rf212processflag=arg
#else
#define RF212PROCESSFLAG(arg)
#endif
PROCESS_THREAD(rf212_process, ev, data)
{
	int len;
	PROCESS_BEGIN();
		RF212PROCESSFLAG(99);

		while(1)
		{
			PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
			RF212PROCESSFLAG(42);
#if RF212_TIMETABLE_PROFILING
			TIMETABLE_TIMESTAMP(rf212_timetable, "poll");
#endif /* RF230_TIMETABLE_PROFILING */

			pending = 0;

			packetbuf_clear();
			len = rf212_read(packetbuf_dataptr(), PACKETBUF_SIZE);
			RF212PROCESSFLAG(1);
			if(len > 0)
			{
				packetbuf_set_datalen(len);
				RF212PROCESSFLAG(2);
				NETSTACK_RDC.input();
#if RF212_TIMETABLE_PROFILING
				TIMETABLE_TIMESTAMP(rf212_timetable, "end");
				timetable_aggregate_compute_detailed(&aggregate_time,
						&rf212_timetable);
				timetable_clear(&rf212_timetable);
#endif /* RF212_TIMETABLE_PROFILING */
			}
			else
			{
#if RADIOSTATS
				RF212_receivefail++;
#endif
			}
		}

		PROCESS_END();
	}

	/*---------------------------------------------------------------------------*/
	/*
	 * This routine is called by the radio receive interrupt in hal.c
	 * It just sets the poll flag for the rf212 process.
	 */
#if RF212_CONF_TIMESTAMPS
	static volatile rtimer_clock_t interrupt_time;
	static volatile int interrupt_time_set;
#endif /* RF212_CONF_TIMESTAMPS */
#if RF212_TIMETABLE_PROFILING
#define rf212_timetable_size 16
	TIMETABLE(rf212_timetable);
	TIMETABLE_AGGREGATE(aggregate_time, 10);
#endif /* RF212_TIMETABLE_PROFILING */
void rf212_interrupt(void)
{
#if RF212_CONF_TIMESTAMPS
	interrupt_time = timesynch_time();
	interrupt_time_set = 1;
#endif /* RF212_CONF_TIMESTAMPS */

	process_poll(&rf212_process);

#if RF212_TIMETABLE_PROFILING
	timetable_clear(&rf212_timetable);
	TIMETABLE_TIMESTAMP(rf212_timetable, "interrupt");
#endif /* RF212_TIMETABLE_PROFILING */
	pending = 1;
	return;
}
/* The frame is buffered to rxframe in the interrupt routine in hal.c */
hal_rx_frame_t rxframe;
/*---------------------------------------------------------------------------*/
int rf212_read(void *buf, unsigned short bufsize)
{

	uint8_t *framep;
	// uint8_t footer[2];
	uint8_t len;
#if RF212_CONF_CHECKSUM
	uint16_t checksum;
#endif /* RF212_CONF_CHECKSUM */
#if RF212_CONF_TIMESTAMPS
	struct timestamp t;
#endif /* RF212_CONF_TIMESTAMPS */

	PRINTF("rf212_read: %u bytes lqi %u crc %u ed %u \n",rxframe.length,rxframe.lqi,rxframe.crc,rxframe.ed);
#if DEBUG>1
	for (len=0;len<rxframe.length;len++) PRINTF(" %x",rxframe.data[len]);PRINTF("\n");
#endif
	if (rxframe.length == 0)
	{
		return 0;
	}

#if RF212_CONF_TIMESTAMPS
	bomb
	if(interrupt_time_set)
	{
		rf212_time_of_arrival = interrupt_time;
		interrupt_time_set = 0;
	}
	else
	{
		rf212_time_of_arrival = 0;
	}
	rf212_time_of_departure = 0;
#endif /* RF212_CONF_TIMESTAMPS */
	GET_LOCK();
	//  if(rxframe.length > RF212_MAX_PACKET_LEN) {
	//    // Oops, we must be out of sync.
	//  flushrx();
	//    RIMESTATS_ADD(badsynch);
	//    RELEASE_LOCK();
	//    return 0;
	//  }

	//hal returns two extra bytes containing the checksum
	//below works because auxlen is 2
	len = rxframe.length;
	if (len <= AUX_LEN)
	{
		// flushrx();
		RIMESTATS_ADD(tooshort);
		RELEASE_LOCK();
		return 0;
	}

	if (len - AUX_LEN > bufsize)
	{
		//  flushrx();
		RIMESTATS_ADD(toolong);
		RELEASE_LOCK();
		return 0;
	}
	/* Transfer the frame, stripping the checksum */
	framep = &(rxframe.data[0]);
	memcpy(buf, framep, len - AUX_LEN);
	/* Clear the length field to allow buffering of the next packet */
	rxframe.length = 0;
	// framep+=len-AUX_LEN+2;

#if RADIOSTATS
	RF212_receivepackets++;
#endif

#if RF212_CONF_CHECKSUM
	bomb
	memcpy(&checksum,framep,CHECKSUM_LEN);
	framep+=CHECKSUM_LEN;
#endif /* RF212_CONF_CHECKSUM */
#if RF212_CONF_TIMESTAMPS
	bomb
	memcpy(&t,framep,TIMESTAMP_LEN);
	framep+=TIMESTAMP_LEN;
#endif /* RF212_CONF_TIMESTAMPS */
	//  memcpy(&footer,framep,FOOTER_LEN);

#if RF212_CONF_CHECKSUM
	bomb
	if(checksum != crc16_data(buf, len - AUX_LEN, 0))
	{
		PRINTF("rf212: checksum failed 0x%04x != 0x%04x\n",
				checksum, crc16_data(buf, len - AUX_LEN, 0));
	}

	if(footer[1] & FOOTER1_CRC_OK &&
			checksum == crc16_data(buf, len - AUX_LEN, 0))
	{
#else
	if (rxframe.crc)
	{
#endif /* RF212_CONF_CHECKSUM */

		/*
		 packetbuf_copyfrom(parsed_frame->payload, parsed_frame->payload_length);
		 packetbuf_set_datalen(parsed_frame->payload_length);

		 memcpy(dest_reversed, (uint8_t *)parsed_frame->dest_addr, UIP_LLADDR_LEN);
		 memcpy(src_reversed, (uint8_t *)parsed_frame->src_addr, UIP_LLADDR_LEN);

		 //Change addresses to expected byte order
		 byte_reverse((uint8_t *)dest_reversed, UIP_LLADDR_LEN);
		 byte_reverse((uint8_t *)src_reversed, UIP_LLADDR_LEN);

		 packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, (const rimeaddr_t *)dest_reversed);
		 packetbuf_set_addr(PACKETBUF_ADDR_SENDER, (const rimeaddr_t *)src_reversed);

		 */
#if RADIOSTATS
		RF212_rsigsi=hal_subregister_read( SR_RSSI );
#endif      
		packetbuf_set_attr(PACKETBUF_ATTR_RSSI, hal_subregister_read(SR_RSSI));
		packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, rxframe.lqi);

		RIMESTATS_ADD(llrx);
#if RF212_CONF_TIMESTAMPS
		bomb
		rf212_time_of_departure =
		t.time +
		setup_time_for_transmission +
		(total_time_for_transmission * (len - 2)) / total_transmission_len;

		rf212_authority_level_of_sender = t.authority_level;

		packetbuf_set_attr(PACKETBUF_ATTR_TIMESTAMP, t.time);
#endif /* RF212_CONF_TIMESTAMPS */

	}
	else
	{
		PRINTF("rf212: Bad CRC\n");

#if RADIOSTATS
		RF212_receivefail++;
#endif

		RIMESTATS_ADD(badcrc);
		len = AUX_LEN;
	}
	// if (?)
	/* Another packet has been received and needs attention. */
	//    process_poll(&rf212_process);
	//  }

	RELEASE_LOCK();

	if (len < AUX_LEN)
	{
		return 0;
	}
#ifdef RF212BB_HOOK_RX_PACKET
	if (RF212BB_HOOK_RX_PACKET(buf,len)) //don't process pakets in raw-mode
		return 0;
#endif

	/* Here return just the data length. The checksum is however still in the buffer for packet sniffing */
	return len - AUX_LEN;
}
/*---------------------------------------------------------------------------*/
void rf212_set_txpower(uint8_t power)
{
	if (radio_is_sleeping() == true)
	{
		PRINTF("rf212_set_txpower:Sleeping");
	}
	else
	{
		hal_subregister_write(SR_TX_PWR,power) ;
	}

}
/*---------------------------------------------------------------------------*/
int rf212_get_txpower(void)
{
	if (radio_is_sleeping() == true)
	{
		PRINTF("rf212_get_txpower:Sleeping");
		return 0;
	}
	else
	{
		return hal_subregister_read(SR_TX_PWR);
	}
}
/*---------------------------------------------------------------------------*/
int rf212_rssi(void)
{
	int rssi;
	int radio_was_off = 0;

	/*The RSSI measurement should only be done in RX_ON or BUSY_RX.*/
	if (!RF212_radio_on)
	{
		radio_was_off = 1;
		rf212_on();
	}

	rssi = (int) ((signed char) hal_subregister_read(SR_RSSI));

	if (radio_was_off)
	{
		rf212_off();
	}
	return rssi;
}
/*---------------------------------------------------------------------------*/
static int rf212_prepare(const void *payload, unsigned short payload_len)
{
	//TODO
	return 0;
}
/*---------------------------------------------------------------------------*/
static int rf212_transmit(unsigned short payload_len)
{
	//TODO
	return 0;
}
/*---------------------------------------------------------------------------*/
uint8_t rf212_generate_random_byte(void)
{
	uint8_t i;
	uint8_t rnd = 0;

	AVR_ENTER_CRITICAL_REGION();
		uint8_t old_trx_state = radio_get_trx_state();
		uint8_t old_rx_pdt_dis = hal_subregister_read(SR_RX_PDT_DIS);
		radio_set_trx_state(RX_ON); // Random Number generator works only in basic TRX states
		hal_subregister_write(SR_RX_PDT_DIS, 0); //also the preamble detector hast to be enabled

				for (i = 0; i < 4; i++) //Register RND_VALUE contains a 2-bit random value

				{
					rnd |= hal_subregister_read(SR_RND_VALUE)<<(2*i);
				}

				hal_subregister_write(SR_RX_PDT_DIS, old_rx_pdt_dis);
				radio_set_trx_state(old_trx_state);
				AVR_LEAVE_CRITICAL_REGION();
				return rnd;
			}

void rf212_generate_key(uint8_t* key)
{
	uint8_t i, j;

	AVR_ENTER_CRITICAL_REGION();
		uint8_t old_trx_state = radio_get_trx_state();
		uint8_t old_rx_pdt_dis = hal_subregister_read(SR_RX_PDT_DIS);
		radio_set_trx_state(RX_ON); // Random Number generator works only in basic TRX states
		hal_subregister_write(SR_RX_PDT_DIS, 0); //also the preamble detector hast to be enabled

				for (i = 0; i < 16; i++)
				{
					key[i] = 0;
					for (j = 0; j < 4; j++) //Register RND_VALUE contains a 2-bit random value

				{
					key[i] |= hal_subregister_read(SR_RND_VALUE)<<(2*j);
				}
			}
			hal_subregister_write(SR_RX_PDT_DIS, old_rx_pdt_dis);
			radio_set_trx_state(old_trx_state);
			AVR_LEAVE_CRITICAL_REGION();
		}
		/*---------------------------------------------------------------------------*/
void rf212_key_setup(uint8_t *key)
{
	uint8_t aes_mode = 0x10; //see 9.1.3 in rf212 manual
	hal_sram_write(AES_CTRL, 1, &aes_mode);
	hal_sram_write(AES_KEY_START, 16, key);
	//TODO: combining of setting the control register and the key will increase speed
}
/*---------------------------------------------------------------------------*/
uint8_t rf212_cipher(uint8_t *data)
{
	uint8_t aes_mode_dir = 0x00; //ECB mode, see 9.1.4.1 in rf212 manual and AES encryption, see 9.1.4.1 in rf212 manual
	uint8_t aes_request = 0x80;
	uint8_t aes_status = 0x00;

	hal_sram_write(AES_CTRL, 1, &(aes_mode_dir));
	hal_sram_write(AES_KEY_START, 16, data);
	hal_sram_write(AES_CTRL_MIRROR, 1, &aes_request);

	delay_us(24);
	while (((aes_status & 0x01) != 0x01) && ((aes_status & 0x80) != 0x80))
		hal_sram_read(AES_STATUS, 1, &aes_status); // check for finished security operation
	if ((aes_status & 0x80) != 0x80)
	{
		hal_sram_read(AES_KEY_START, 16, data); //read encrypted data
		return 0;
	}
	else
		PRINTF("rf212: cipher error\n");
	return 1; //AES module error
}

int rf212_send(const void *payload, unsigned short payload_len)
{
	//  int i;
	uint8_t total_len, buffer[RF212_MAX_TX_FRAME_LENGTH], *pbuf;
#if RF212_CONF_TIMESTAMPS
	struct timestamp timestamp;
#endif /* RF212_CONF_TIMESTAMPS */
#if RF212_CONF_CHECKSUM
	uint16_t checksum;
#endif /* RF212_CONF_CHECKSUM */

#if RADIOSTATS
	RF212_sendpackets++;
#endif
	GET_LOCK();

	/* KM 2011-02-21 not supported any more, transmit power is static
	 if(packetbuf_attr(PACKETBUF_ATTR_RADIO_TXPOWER) > 0) {
	 rf212_set_txpower(packetbuf_attr(PACKETBUF_ATTR_RADIO_TXPOWER) - 1);
	 } else {
	 rf212_set_txpower(TX_PWR_17_2DBM);
	 }
	 */

	RIMESTATS_ADD(lltx);

#if RF212_CONF_CHECKSUM
	checksum = crc16_data(payload, payload_len, 0);
#endif /* RF212_CONF_CHECKSUM */
	total_len = payload_len + AUX_LEN;
	/*Check function parameters and current state.*/
	if (total_len > RF212_MAX_TX_FRAME_LENGTH)
	{
#if RADIOSTATS
		RF212_sendfail++;
#endif   
		return -1;
	}
	pbuf = &buffer[0];
	memcpy(pbuf, payload, payload_len);
	pbuf += payload_len;

#if RF212_CONF_CHECKSUM
	memcpy(pbuf,&checksum,CHECKSUM_LEN);
	pbuf+=CHECKSUM_LEN;
#endif /* RF212_CONF_CHECKSUM */

#if RF212_CONF_TIMESTAMPS
	timestamp.authority_level = timesynch_authority_level();
	timestamp.time = timesynch_time();
	memcpy(pbuf,&timestamp,TIMESTAMP_LEN);
	pbuf+=TIMESTAMP_LEN;
#endif /* RF212_CONF_TIMESTAMPS */

#ifdef RF212BB_HOOK_TX_PACKET
	RF212BB_HOOK_TX_PACKET(buffer, total_len);
#endif
	/* Wait for any previous transmission to finish. */
	rf212_waitidle();

	//GH: 11.04.2011 no transition to TRX_OFF needed
	//hal_subregister_write(SR_TRX_CMD, CMD_FORCE_TRX_OFF);
	//delay_us(TIME_P_ON_TO_TRX_OFF);

	if (bootloader_mode == 1)
		radio_set_trx_state(PLL_ON); //enable auto ack
	else
		radio_set_trx_state(TX_ARET_ON); //enable auto ack


	PRINTF("rf212: sending %d bytes\n", payload_len);
	//print packet
#if DEBUG >1
	int len;
	for (len=0;len<total_len;len++) PRINTF(" %x",buffer[len]);PRINTF("\n");
#endif

	hal_frame_write(buffer, total_len);
	/* Toggle the SLP_TR pin to initiate the frame transmission. */hal_set_slptr_high();
	hal_set_slptr_low();
	if (1)
	{
#if RF212_CONF_TIMESTAMPS
		rtimer_clock_t txtime = timesynch_time();
#endif /* RF212_CONF_TIMESTAMPS */

		if (RF212_radio_on)
		{
			ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
		}
		ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);

		/* We wait until transmission has ended so that we get an
		 accurate measurement of the transmission time.*/
		rf212_waitidle();

#if RF212_CONF_TIMESTAMPS
		setup_time_for_transmission = txtime - timestamp.time;

		if(num_transmissions < 10000)
		{
			total_time_for_transmission += timesynch_time() - txtime;
			total_transmission_len += total_len;
			num_transmissions++;
		}
#endif /* RF212_CONF_TIMESTAMPS */

#ifdef ENERGEST_CONF_LEVELDEVICE_LEVELS
		ENERGEST_OFF_LEVEL(ENERGEST_TYPE_TRANSMIT,rf212_get_txpower());
#endif
		ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
		if (RF212_radio_on)
		{
			ENERGEST_ON(ENERGEST_TYPE_LISTEN);
		}

		RELEASE_LOCK();

		//GH: transition to receive mode should be done in interrupt service otherwise sent frame will be read from the frame buffer
		//radio_set_trx_state(RX_AACK_ON);//Re-enable receive mode
		return 0;
	}

	/* If we are using WITH_SEND_CCA, we get here if the packet wasn't
	 transmitted because of other channel activity. */RIMESTATS_ADD(contentiondrop);
	PRINTF("rf212: do_send() transmission never started\n");
#if RADIOSTATS
	RF212_sendfail++;
#endif     
	RELEASE_LOCK();
	return -3; /* Transmission never started! */
}/*---------------------------------------------------------------------------*/
static void rf212_calibrate()
{
	hal_subregister_write(SR_FTN_START, 1);
	// recalibration takes at max. 25Âµs
			delay_us(25);
		}/*---------------------------------------------------------------------------*/
void rf212_set_promiscuous_mode(uint8_t onoff, uint8_t * mac_address)
{
	if (onoff > 0)
	{
		//enable promiscuous mode
		hal_subregister_write(SR_AACK_PROM_MODE, 1);
		hal_subregister_write(SR_AACK_DIS_ACK, 1); //disable acknowledgment generation
				hal_subregister_write(SR_RX_SAFE_MODE, 1);
				hal_subregister_write(SR_I_AM_COORD, 0);
				hal_subregister_write(SR_SLOTTED_OPERATON, 0);
				//		hal_subregister_write(SR_FVN_MODE, 3); // acknowledge all frames
				hal_subregister_write(SR_AACK_UPLD_RES_FT, 0); // disable reserved frames
				hal_subregister_write(SR_AACK_FLTR_RES_FT, 0); //no frame filter
				uint8_t prom_mode_mac_address[8] =
				{	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
				rf212_set_pan_addr(0x0000, 0x0000, prom_mode_mac_address);
				promiscuous_mode = 1;
				radio_set_trx_state(RX_ON);
			}
			else
			{
				//disable promiscuous mode
				hal_subregister_write(SR_AACK_PROM_MODE, 0);
				hal_subregister_write(SR_AACK_DIS_ACK, 0);
				hal_subregister_write(SR_RX_SAFE_MODE, 0);
				rf212_set_pan_addr(IEEE802154_PANID, 0x0000, mac_address);
				promiscuous_mode = 0;
			}
		}/*---------------------------------------------------------------------------*/

bool rf212_is_ready_to_send()
{
	switch (radio_get_trx_state())
	{
	case BUSY_TX:
	case BUSY_TX_ARET:
		return false;
	}

	return true;
}/*---------------------------------------------------------------------------*/

int rf212_init(void)
{
	/* Wait in case VCC just applied */delay_us(TIME_TO_ENTER_P_ON);

	/* Calibrate oscillator */
	// calibrate_rc_osc_32k();

	/* Initialize Hardware Abstraction Layer. */
	hal_init();

	/* Do full rf212 Reset */
	/* GH: 06.04.2011 changed to recommended programming sequence see page 166
	 hal_set_rst_low();
	 hal_set_slptr_low();
	 delay_us(TIME_RESET);
	 hal_set_rst_high();
	 */
	//set input pins to the default operating values
	hal_set_slptr_low();
	hal_set_rst_high();
	HAL_SS_HIGH();
	//wait for at least 400us
	delay_us(400);
	//reset the transceiver
	hal_set_rst_low();
	delay_us(TIME_RESET);
	hal_set_rst_high();
	delay_us(26);

#if RAVEN_REVISION == HEXABUS_USB
	//set CLKM rate to 8 MHz, note: CLKM is clock of AT90USB1287
	hal_register_write(RG_TRX_CTRL_0, 0x04);
#else
	//disable CLKM on HEXABUS_SOCKET
	hal_register_write(RG_TRX_CTRL_0, 0x00);
#endif

	/* Force transition to TRX_OFF. */
	hal_subregister_write(SR_TRX_CMD, CMD_FORCE_TRX_OFF);
	delay_us(TIME_P_ON_TO_TRX_OFF);

	/* Verify that it is a supported version */
	uint8_t tvers = hal_register_read(RG_VERSION_NUM);
	uint8_t tmanu = hal_register_read(RG_MAN_ID_0);
	uint8_t tpartnum = hal_register_read(RG_PART_NUM);

	if ((tvers != RF212_REVA))
		PRINTF("rf212: Unsupported version %u\n",tvers);
	if (tmanu != SUPPORTED_MANUFACTURER_ID)
		PRINTF("rf212: Unsupported manufacturer ID %u\n",tmanu);
	if (tpartnum != SUPPORTED_PART_NUMBER)
		PRINTF("rf212: Unsupported part number %u\n",tpartnum);

	PRINTF("rf212: Version %u, ID %u\n",tvers,tmanu);

	// set OQSPK modulation with 100KB/s (802.15.4 regulation Europe)
	hal_register_write(RG_TRX_CTRL_2, TRX_CTRL2_OQPSK_100KB);
	/*set frequency to band 0 and use channel 0
	 (802.15.4 regulation Europe, 868 MHz) */
	hal_register_write(RG_CC_CTRL_1, CC_BAND);
	hal_subregister_write(SR_CHANNEL, CC_CHANNEL);

	/*configure listen before talk (LBT) (rf212 manual, page 88ff):
	 * CCA Mode: Do not transmit if energy detection (ED) is above threshold or
	 * 	carrier sense (CS) sensed a signal
	 * ED Threshold: According to European regulations, the ED threshold for LBT
	 * 	is -82 dBm (page 89, ref212 manual). The threshold can be
	 * 	calculated with "RSSI_BASE_VAL + 2.07 * CCA_ED_THRES" [dBm]
	 * CSMA_LBT_Mode: the transceiver can either operate in 802.15.4 CSMA mode
	 * 	or in the LBT mode according to European regulations. In LBT mode the
	 * 	radio will stay in TX_ARET mode until the channel is free, so the
	 * 	MAX_CSMA_RETRIES are neglected.
	 */
	hal_subregister_write(SR_CCA_MODE, CCA_ED_OR_CS);
	hal_subregister_write(SR_CCA_ED_THRES, CCA_ED_MAX_THRESHOLD_BPSK_20);
	hal_subregister_write(SR_CSMA_LBT_MODE, CCA_LBT_ENABLED);

	/* configure the maximal number of frame retries  */
	hal_subregister_write(SR_MAX_FRAME_RETRIES, RF212_MAX_FRAME_RETRIES);

	/* 802.15.4 specific defines. These are the default values */
	hal_subregister_write(SR_RX_SAFE_MODE, 0);
	hal_subregister_write(SR_I_AM_COORD, 0);
	hal_subregister_write(SR_SLOTTED_OPERATON, 0);
	hal_subregister_write(SR_FVN_MODE, 1); // acknowledge version 1 and version 2
			hal_subregister_write(SR_AACK_UPLD_RES_FT, 0); // block reserved frames
			hal_subregister_write(SR_AACK_FLTR_RES_FT, 0); //no frame filter for reserved frames


			/*set transmit power
			 * see AT86RF212 Manual Page 107 (EU2, Boost mode enabled (higher energy
			 * consumption, but also higher output power), 5dBm),
			 * maximum power that the transceiver can provide without inferring with
			 * the regulations on the 868 MHz band in Europe
			 */
			//hal_register_write(RG_PHY_TX_PWR, RF212_TX_PWR_5DBM_BOOST_MODE|RF212_ENABLE_PA_BOOST);
			//GH: 09.05.2011 in EMC-test used TX_PWR values
			hal_register_write(RG_PHY_TX_PWR, 0x62);

			/* set sensitivity of the transceiver, '0' means maximum sensitivity.
			 * If too many "false" receive interrupts are generated, the sensitivity can
			 * be reduced and this setting can be adjusted.
			 */
			hal_register_write(RG_RX_SYN, 0x0);

			// configure the interrupt generation of the radio
			hal_register_write(RG_IRQ_MASK, RF212_SUPPORTED_INTERRUPT_MASK);

			/* Set up number of automatic retries */
			hal_subregister_write(SR_MAX_FRAME_RETRIES, RF212_CONF_AUTORETRIES );

			/* Set up the radio for auto mode operation. Automatic CRC is used and
			 * the radio is set in auto-receive mode. Before a transmission is
			 * started, the radio will be set to TX_ARET mode (automatic retransmission
			 * mode)
			 */

			hal_subregister_write(SR_TX_AUTO_CRC_ON, 1);
			radio_set_trx_state(RX_AACK_ON);

			/* Change default values as recomended in the data sheet, */
			/* correlation threshold = 20, RX bandpass filter = 1.3uA. */
			//  setreg(RF212_MDMCTRL1, CORR_THR(20));
			//  reg = getreg(RF212_RXCTRL1);
			//  reg |= RXBPF_LOCUR;
			//  setreg(RF212_RXCTRL1, reg);


			/* Start the packet receive process */
			process_start(&rf212_process, NULL);
			return 1;
		}

