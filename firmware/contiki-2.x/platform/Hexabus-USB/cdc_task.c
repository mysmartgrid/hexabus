/* This file has been prepared for Doxygen automatic documentation generation.*/
/*! \file cdc_task.c **********************************************************
 *
 * \brief
 *      Manages the CDC-ACM Virtual Serial Port Dataclass for the USB Device
 *
 * \addtogroup usbstick
 *
 * \author 
 *        Colin O'Flynn <coflynn@newae.com>
 *
 ******************************************************************************/
/* Copyright (c) 2008  ATMEL Corporation
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.
   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
/**
 \ingroup usbstick
 \defgroup cdctask CDC Task
 @{
 */

//_____  I N C L U D E S ___________________________________________________


#include "contiki.h"
#include "usb_drv.h"
#include "usb_descriptors.h"
#include "usb_specific_request.h"
#include "cdc_task.h"
#include "serial/uart_usb_lib.h"
#include "rndis/rndis_protocol.h"
#include "rndis/rndis_task.h"
#include "sicslow_ethernet.h"
#if RF230BB
#include "rf230bb.h"
#elif RF212BB
#include "rf212bb.h"
#else
#include "radio.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include "dev/watchdog.h"
//#include "rng.h"

#include "bootloader.h"

#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "dev/leds.h"
#include "bootloader_send.h"
#include "eeprom_variables.h"

#if JACKDAW_CONF_USE_SETTINGS
#include "settings.h"
#endif

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
#define PRINTF printf
#define PRINTF_P printf_P

//_____ M A C R O S ________________________________________________________


#define bzero(ptr,size)	memset(ptr,0,size)

//_____ D E F I N I T I O N S ______________________________________________


#define IAD_TIMEOUT_DETACH 300
#define IAD_TIMEOUT_ATTACH 600

//_____ D E C L A R A T I O N S ____________________________________________


void menu_print(void);
void menu_process(char c);

extern char usb_busy;

//! Counter for USB Serial port
extern U8    tx_counter;

//! previous configuration
static uint8_t previous_uart_usb_control_line_state = 0;

static uint8_t timer = 0;
static struct etimer et;

PROCESS(cdc_process, "CDC serial process");

/**
 * \brief Communication Data Class (CDC) Process
 *
 *   This is the link between USB and the "good stuff". In this routine data
 *   is received and processed by CDC-ACM Class
 */
PROCESS_THREAD(cdc_process, ev, data_proc)
{
	PROCESS_BEGIN();

#if USB_CONF_RS232
	static FILE *rs232_stdout,*usb_stdout;
	rs232_stdout=stdout;
#endif

	while(1) {
		
 		if(Is_device_enumerated()) {
			// If the configuration is different than the last time we checked...
			if((uart_usb_get_control_line_state()&1)!=previous_uart_usb_control_line_state) {
				previous_uart_usb_control_line_state = uart_usb_get_control_line_state()&1;
				static FILE* previous_stdout;
				
				if(previous_uart_usb_control_line_state&1) {
					previous_stdout = stdout;
					uart_usb_init();
					uart_usb_set_stdout();
				//	menu_print(); do this later
				} else {
					stdout = previous_stdout;
				}
#if USB_CONF_RS232
				usb_stdout=stdout;
#endif
			}

			//Flush buffer if timeout
	        if(timer >= 4 && tx_counter!=0 ){
	            timer = 0;
	            uart_usb_flush();
	        } else {
				timer++;
			}

#if USB_CONF_RS232
			stdout=usb_stdout;
#endif
			while (uart_usb_test_hit()){
  		  	   menu_process(uart_usb_getchar());   // See what they want
            }
#if USB_CONF_RS232
            if (usbstick_mode.debugOn) {
			  stdout=rs232_stdout;
			} else {
			  stdout=NULL;
			}
#endif
		}//if (Is_device_enumerated())



		if (USB_CONFIG_HAS_DEBUG_PORT(usb_configuration_nb)) {
			etimer_set(&et, CLOCK_SECOND/80);
		} else {
			etimer_set(&et, CLOCK_SECOND);
		}

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));	
		
	} // while(1)

	PROCESS_END();
}

/**
 \brief Get and set a new PAN ID
 */
void cdc_set_pan_id(void)
{
	uint8_t i;
	char pan_id_str[4];
	uint16_t pan_id;
	for (i=0; i < 4; i++) {
		pan_id_str[i] = uart_usb_getchar();
	}
	sscanf(pan_id_str, "%x", &pan_id);
	PRINTF_P(PSTR("\n\rSetting new PAN ID to: %X\n\r"),pan_id);
	eeprom_write_word ((uint16_t *)EE_PAN_ID, pan_id);
	PRINTF_P(PSTR("Reset the USB-Stick so that changes take effect!\n\r"));
}

/**
 \brief Print debug menu
 */
void menu_print(void)
{
		PRINTF_P(PSTR("\n\r*********** Jackdaw Menu **********\n\r"));
		PRINTF_P(PSTR("        [Built "__DATE__"]         \n\r"));
//		PRINTF_P(PSTR("*                                 *\n\r"));
		PRINTF_P(PSTR("*  m        Print current mode    *\n\r"));
		PRINTF_P(PSTR("*  s        Set to sniffer mode   *\n\r"));
		PRINTF_P(PSTR("*  n        Set to network mode   *\n\r"));
		PRINTF_P(PSTR("*  r        set to raw mode       *\n\r"));
		PRINTF_P(PSTR("*  f        Flash a Socket        *\n\r"));
		PRINTF_P(PSTR("*  c        Change PAN ID         *\n\r"));
		PRINTF_P(PSTR("*  R        Reset (via WDT)       *\n\r"));
		PRINTF_P(PSTR("*  G        Generate new random   *\n\r"));
		PRINTF_P(PSTR("*           PAN ID and AES128 key *\n\r"));
		PRINTF_P(PSTR("*  h,?      Print this menu       *\n\r"));
		PRINTF_P(PSTR("*                                 *\n\r"));
		PRINTF_P(PSTR("* Make selection at any time by   *\n\r"));
		PRINTF_P(PSTR("* pressing your choice on keyboard*\n\r"));
		PRINTF_P(PSTR("***********************************\n\r"));
}

#if UIP_CONF_IPV6_RPL
static void
ipaddr_add(const uip_ipaddr_t *addr)
{
  uint16_t a;
  int8_t i, f;
  for(i = 0, f = 0; i < sizeof(uip_ipaddr_t); i += 2) {
    a = (addr->u8[i] << 8) + addr->u8[i + 1];
    if(a == 0 && f >= 0) {
      if(f++ == 0) PRINTF_P(PSTR("::"));
    } else {
      if(f > 0) {
        f = -1;
      } else if(i > 0) {
	    PRINTF_P(PSTR(":"));
      }
	  PRINTF_P(PSTR("%x"),a);
    }
  }
}
#endif

/**
 \brief Process incomming char on debug port
 */
void menu_process(char c)
{

		uint8_t i;

        /* Any attempt to read an RF230 register in sneeze mode (e.g. rssi) will hang the MCU */
        /* So convert any command into a sneeze off */

        if (usbstick_mode.sneeze) c='S';

		switch(c) {
			case '\r':
			case '\n':
				break;

			case 'h':
			case '?':
				menu_print();
				break;
			case 's':
				PRINTF_P(PSTR("Jackdaw now in sniffer mode\n\r"));
				rf212_set_promiscuous_mode(1, NULL);
				usbstick_mode.sendToRf = 0;
				usbstick_mode.translate = 0;
				break;
				
			case 'r':
				PRINTF_P(PSTR("Jackdaw now in raw mode\n\r"));
				usbstick_mode.raw = 1;
				break;				

			case 'n':
				PRINTF_P(PSTR("Jackdaw now in network mode\n\r"));
				extern uint64_t macLongAddr;
				rf212_set_promiscuous_mode(0,(uint8_t *)&macLongAddr);
				usbstick_mode.sendToRf = 1;
				usbstick_mode.translate = 1;
				usbstick_mode.raw = 0;
				break;

			case 'm':
				PRINTF_P(PSTR("Currently Jackdaw:\n\r  * Will "));
				if (usbstick_mode.sendToRf == 0) { PRINTF_P(PSTR("not "));}
				PRINTF_P(PSTR("send data over RF\n\r  * Will "));
				if (usbstick_mode.translate == 0) { PRINTF_P(PSTR("not "));}
				PRINTF_P(PSTR("change link-local addresses inside IP messages\n\r  * Will "));
				if (usbstick_mode.sicslowpan == 0) { PRINTF_P(PSTR("not "));}
				PRINTF_P(PSTR("decompress 6lowpan headers\n\r  * Will "));
				if (usbstick_mode.raw == 0) { PRINTF_P(PSTR("not "));}

#if USB_CONF_RS232
				PRINTF_P(PSTR("Output raw 802.15.4 frames\n\r  * Will "));
				if (usbstick_mode.debugOn == 0) { PRINTF_P(PSTR("not "));}
				PRINTF_P(PSTR("Output RS232 debug strings\n\r"));
#else
				PRINTF_P(PSTR("Output raw 802.15.4 frames\n\r"));
#endif

				PRINTF_P(PSTR("  * USB Ethernet MAC: %02x:%02x:%02x:%02x:%02x:%02x\n"),
					((uint8_t *)&usb_ethernet_addr)[0],
					((uint8_t *)&usb_ethernet_addr)[1],
					((uint8_t *)&usb_ethernet_addr)[2],
					((uint8_t *)&usb_ethernet_addr)[3],
					((uint8_t *)&usb_ethernet_addr)[4],
					((uint8_t *)&usb_ethernet_addr)[5]
				);
				extern uint64_t macLongAddr;
				PRINTF_P(PSTR("  * 802.15.4 EUI-64: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n"),
					((uint8_t *)&macLongAddr)[0],
					((uint8_t *)&macLongAddr)[1],
					((uint8_t *)&macLongAddr)[2],
					((uint8_t *)&macLongAddr)[3],
					((uint8_t *)&macLongAddr)[4],
					((uint8_t *)&macLongAddr)[5],
					((uint8_t *)&macLongAddr)[6],
					((uint8_t *)&macLongAddr)[7]
				);

				PRINTF_P(PSTR("  * Configuration: %d, USB<->ETH is "), usb_configuration_nb);
				if (usb_eth_is_active == 0) PRINTF_P(PSTR("not "));
				PRINTF_P(PSTR("active\n\r"));
				PRINTF_P(PSTR("  * Promiscuous mode is "));
				extern uint8_t promiscuous_mode;
				if (promiscuous_mode == 0) PRINTF_P(PSTR("not "));
				PRINTF_P(PSTR("active\n\r"));
				break;

			case 'R':
				{
					PRINTF_P(PSTR("Resetting...\n\r"));
					uart_usb_flush();
					leds_on(LEDS_ALL);
					for(i = 0; i < 10; i++)_delay_ms(100);
					Usb_detach();
					for(i = 0; i < 20; i++)_delay_ms(100);
					watchdog_reboot();
				}
				break;
			case 'G':
				{
					PRINTF_P(PSTR("Generating new random PAN ID and AES128 key...\n\r"));
					eeprom_write_byte ((uint8_t *)EE_FIRST_RUN_FLAG, 1);
					PRINTF_P(PSTR("Resetting...\n\r"));
					uart_usb_flush();
					leds_on(LEDS_ALL);
					for(i = 0; i < 10; i++)_delay_ms(100);
					Usb_detach();
					for(i = 0; i < 20; i++)_delay_ms(100);
					watchdog_reboot();
				}
				break;

			case 'f':
				stdout = 0;
				bootloader_send_data();
				uart_usb_set_stdout();
				break;

			case 'c':
				PRINTF_P(PSTR("\n\rInsert new PAN ID in HEX-format (i.e. ABCD):\n\r"));
				cdc_set_pan_id();
				break;

			default:
				PRINTF_P(PSTR("%c is not a valid option! h for menu\n\r"), c);
				break;
		}

	return;

}

/** @}  */
