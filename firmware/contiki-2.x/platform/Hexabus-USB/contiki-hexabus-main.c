/*
 * Copyright (c) 2006, Technical University of Munich
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
 * \file
 *         Contiki 2.4 kernel for Jackdaw USB stick
 *
 * \author
 *         Simon Barner <barner@in.tum.de>
 *         David Kopf <dak664@embarqmail.com>
 */

#define DEBUG 0
#if DEBUG
#define PRINTD(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTD(...)
#endif

#include <avr/pgmspace.h>
#include <avr/fuse.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

#include "lib/mmem.h"
#include "loader/symbols-def.h"
#include "loader/symtab.h"

#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"
#include "contiki-hexabus.h"
#include "leds.h"

/* Set ANNOUNCE to send boot messages to USB or RS232 serial port */
#define ANNOUNCE 1

/* But only if a serial port exists */
#if USB_CONF_SERIAL||USB_CONF_RS232
#define PRINTA(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTA(...)
#endif

#include "usb_task.h"
#if USB_CONF_SERIAL
#include "cdc_task.h"
#endif
#if USB_CONF_RS232
#include "dev/rs232.h"
#endif

#include "rndis/rndis_task.h"
#if USB_CONF_STORAGE
#include "storage/storage_task.h"
#endif

#include "dev/watchdog.h"
#include "dev/usb/usb_drv.h"

#include "radio/rf212bb/rf212bb.h"
#include "net/mac/frame802154.h"

#include "lib/random.h"

//HEXABUS includes
#include "button.h"
#include "eeprom_variables.h"
#include "bootloader_send.h"

#define UIP_IP_BUF ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
rimeaddr_t macLongAddr;
#define	tmp_addr	macLongAddr

uint8_t encryption_enabled = 1; //global variable for AES encryption, in provisioning this variable is set to 0

#if UIP_CONF_IPV6_RPL
/*---------------------------------------------------------------------------*/
/*---------------------------------  RPL   ----------------------------------*/
/*---------------------------------------------------------------------------*/
/* TODO: Put rpl code into another file, once it stabilizes                  */
/* Set up fallback interface links to direct stack tcpip output to ethernet  */
static void
init(void)
{
}
void mac_LowpanToEthernet(void);
static void
output(void)
{
	//  if(uip_ipaddr_cmp(&last_sender, &UIP_IP_BUF->srcipaddr)) {
	/* Do not bounce packets back over USB if the packet was received from USB */
	//    PRINTA("JACKDAW router: Destination off-link but no route\n");
	// } else {
	PRINTD("SUT: %u\n", uip_len);
	mac_LowpanToEthernet(); //bounceback trap is done in lowpanToEthernet
	//  }
}
const struct uip_fallback_interface rpl_interface =
{
	init, output
};

#if RPL_BORDER_ROUTER
#include "net/rpl/rpl.h"

uint16_t dag_id[] PROGMEM =
{	0x1111, 0x1100, 0, 0, 0, 0, 0, 0x0011};

PROCESS(border_router_process, "RPL Border Router");
PROCESS_THREAD(border_router_process, ev, data)
{

	PROCESS_BEGIN();

	PROCESS_PAUSE();

	{	rpl_dag_t *dag;
		char buf[sizeof(dag_id)];
		memcpy_P(buf,dag_id,sizeof(dag_id));
		dag = rpl_set_root((uip_ip6addr_t *)buf);

		/* Assign separate addresses to the jackdaw uip stack and the host network interface, but with the same prefix */
		/* E.g. bbbb::200 to the jackdaw and bbbb::1 to the host network interface with $ip -6 address add bbbb::1/64 dev usb0 */
		/* Otherwise the host will trap packets intended for the jackdaw, just as the jackdaw will trap RF packets intended for the host */
		/* $ifconfig usb0 -arp on Ubuntu to skip the neighbor solicitations. Add explicit neighbors on other OSs */
		if(dag != NULL)
		{
			PRINTD("created a new RPL dag\n");

#if UIP_CONF_ROUTER_RECEIVE_RA
			//Contiki stack will shut down until assigned an address from the interface RA
			//Currently this requires changes in the core rpl-icmp6.c to pass the link-local RA broadcast

#else
			uip_ip6addr_t ipaddr;
			uip_ip6addr(&ipaddr, 0xbbbb, 0, 0, 0, 0, 0, 0, 0x200);
			uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
			rpl_set_prefix(dag, &ipaddr, 64);
#endif
		}
	}
	/* The border router runs with a 100% duty cycle in order to ensure high
	 packet reception rates. */
	// NETSTACK_MAC.off(1);

	while(1)
	{
		PROCESS_YIELD();
		/* Local and global dag repair can be done from the jackdaw menu */
		//   rpl_set_prefix(rpl_get_dag(RPL_ANY_INSTANCE), &ipaddr, 64);
		//   rpl_repair_dag(rpl_get_dag(RPL_ANY_INSTANCE));

	}

	PROCESS_END();
}
#endif /* RPL_BORDER_ROUTER */

#endif /* UIP_CONF_IPV6_RPL */

/*-------------------------------------------------------------------------*/
/*----------------------Configuration of the .elf file---------------------*/
#if 1
/* The proper way to set the signature is */
#include <avr/signature.h>
#else
/* Older avr-gcc's may not define the needed SIGNATURE bytes. Do it manually if you get an error */
typedef struct
{	const unsigned char B2;const unsigned char B1;const unsigned char B0;}__signature_t;
#define SIGNATURE __signature_t __signature __attribute__((section (".signature")))
SIGNATURE =
{
	.B2 = 0x82,//SIGNATURE_2, //AT90USB1287
	.B1 = 0x97,//SIGNATURE_1, //128KB flash
	.B0 = 0x1E,//SIGNATURE_0, //Atmel
};
#endif

FUSES =
{ .low = 0xd0, .high = 0x90, .extended = 0xff, };

volatile uint8_t ee_mem[EESIZE] EEMEM =
{
	0x00, // ee_dummy
	0x02, 0x11, 0x22, 0xff, 0xfe, 0x33, 0x44, 0x55, // ee_mac_addr
	0x00, 0x00, // ee_pan_addr
	0xCD, 0xAB, // ee_pan_id
	0x00, // ee_bootloader_flag
	0x00, 0x00, //ee_bootloader_crc
	0x01, //ee_first_run
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //ee_encryption_key
};

bool get_eui64_from_eeprom(uint8_t macptr[8])
{
	eeprom_read_block((void *) macptr, (const void *) EE_MAC_ADDR,
			EE_MAC_ADDR_SIZE);
	return macptr[0] != 0xFF;
}

void get_aes128key_from_eeprom(uint8_t keyptr[16])
{
	eeprom_read_block((void *) keyptr, (const void *) EE_ENCRYPTION_KEY,
			EE_ENCRYPTION_KEY_SIZE);
}

uint16_t get_panid_from_eeprom(void)
{
	return eeprom_read_word((const void *) EE_PAN_ID);
}

uint16_t get_panaddr_from_eeprom(void)
{
	return eeprom_read_word((const void *) EE_PAN_ADDR);
}

void generate_random_pan_id_and_aes_key(void)
{
	/* PAN ID and AES key is only generated once at first startup */
	if (eeprom_read_byte((const void *) EE_FIRST_RUN_FLAG))
	{
		uint8_t key[16];
		//AES key
		rf212_generate_key(key);
		eeprom_write_block(key, (void *) EE_ENCRYPTION_KEY,
				EE_ENCRYPTION_KEY_SIZE);

		//PAN ID
		uint16_t rand_pan_id = 0xFFFF;
		//init random number generator with first 2 bytes of AES key
		random_init(eeprom_read_word((const void *) EE_ENCRYPTION_KEY));
		//generate random pan_id which is not the broadcast_pan_id and not the bootloader/provisioning pan_id
		while (rand_pan_id == 0xFFFF || rand_pan_id == 0x0001)
		{
			rand_pan_id = random_rand();
		}
		eeprom_write_word((uint16_t *) EE_PAN_ID, rand_pan_id);
		eeprom_write_byte((uint8_t *) EE_FIRST_RUN_FLAG, 0x00);
	}
}

/*-------------------------------------------------------------------------*/
/*-----------------------------Low level initialization--------------------*/
static void initialize(void)
{

	asm volatile ("clr r1");

	watchdog_init();
	watchdog_start();

	/* Clock */
	clock_init();

	/* Initialize hardware */
	init_lowlevel();

#if USB_CONF_RS232
	/* Use rs232 port for serial out (tx, rx, gnd are the three pads behind jackdaw leds */
	rs232_init(RS232_PORT_0, USART_BAUD_57600,USART_PARITY_NONE | USART_STOP_BITS_1 | USART_DATA_BITS_8);
	/* Redirect stdout to second port */
	rs232_redirect_stdout(RS232_PORT_0);
#if ANNOUNCE
	PRINTA("\n\n*******Booting %s*******\n",CONTIKI_VERSION_STRING);
#endif
#endif

	/* rtimer init needed for low power protocols */
	//rtimer_init();

	/* Process subsystem. */
	process_init();

	/* etimer process must be started before USB or ctimer init */
	process_start(&etimer_process, NULL);

	//ctimer_init();
	/* Start radio and radio receive process */
	/* Note this starts RF230 process, so must be done after process_init */NETSTACK_RADIO.init();

	generate_random_pan_id_and_aes_key();

	/* Set addresses BEFORE starting tcpip process */

	memset(&tmp_addr, 0, sizeof(rimeaddr_t));

	if (get_eui64_from_eeprom(tmp_addr.u8))
		;

	//Fix MAC address
	init_net();

#if UIP_CONF_IPV6
	memcpy(&uip_lladdr.addr, &tmp_addr.u8, 8);
#endif

	rf212_set_pan_addr(get_panid_from_eeprom(), get_panaddr_from_eeprom(),
			(uint8_t *) &tmp_addr.u8);

	extern uint16_t mac_dst_pan_id;
	extern uint16_t mac_src_pan_id;
	//set pan_id for frame creation
	mac_dst_pan_id = get_panid_from_eeprom();
	mac_src_pan_id = mac_dst_pan_id;

	rimeaddr_set_node_addr(&tmp_addr);

	/* Initialize stack protocols */
	queuebuf_init();
	NETSTACK_RDC.init();
	NETSTACK_MAC.init();
	NETSTACK_NETWORK.init();

#if ANNOUNCE
	PRINTA("MAC address %x:%x:%x:%x:%x:%x:%x:%x\n\r",tmp_addr.u8[0],tmp_addr.u8[1],tmp_addr.u8[2],tmp_addr.u8[3],tmp_addr.u8[4],tmp_addr.u8[5],tmp_addr.u8[6],tmp_addr.u8[7]);PRINTA("%s %s, channel %u",NETSTACK_MAC.name, NETSTACK_RDC.name,rf212_get_channel());PRINTA("\n");
#endif

#if UIP_CONF_IPV6_RPL
#if RPL_BORDER_ROUTER
	process_start(&tcpip_process, NULL);
	process_start(&border_router_process, NULL);
	PRINTD ("RPL Border Router Started\n");
#else
	process_start(&tcpip_process, NULL);
	PRINTD ("RPL Started\n");
#endif
#if RPL_HTTPD_SERVER
	extern struct process httpd_process;
	process_start(&httpd_process, NULL);
	PRINTD ("Webserver Started\n");
#endif
#endif /* UIP_CONF_IPV6_RPL */

	/* Start USB enumeration */
	process_start(&usb_process, NULL);

	/* Start CDC enumeration, bearing in mind that it may fail */
	/* Hopefully we'll get a stdout for startup messages, if we don't already */
#if USB_CONF_SERIAL
	process_start(&cdc_process, NULL);
	{	unsigned short i;
		for (i=0;i<65535;i++)
		{
			process_run();
			watchdog_periodic();
			if (stdout) break;
		}
#if !USB_CONF_RS232
		PRINTA("\n\n*******Booting %s*******\n",CONTIKI_VERSION_STRING);
#endif
	}
#endif

	/* Start ethernet network and storage process */
	process_start(&usb_eth_process, NULL);

#if USB_CONF_STORAGE
	//  process_start(&storage_process, NULL);
#endif

#if ANNOUNCE
#if USB_CONF_RS232
	PRINTA("Online.\n");
#else
	PRINTA("Online. Type ? for Jackdaw menu.\n");
#endif
#endif

	/* Button Process */
	process_start(&button_pressed_process, NULL);

	leds_on(LEDS_GREEN); //normal state indication
}

/*-------------------------------------------------------------------------*/
/*---------------------------------Main Routine----------------------------*/
int main(void)
{
	/* GCC depends on register r1 set to 0 (?) */
	asm volatile ("clr r1");

	/* Initialize in a subroutine to maximize stack space */
	initialize();

#if DEBUG
	{	struct process *p;
		for(p = PROCESS_LIST();p != NULL; p = ((struct process *)p->next))
		{
			PRINTA("Process=%p Thread=%p  Name=\"%s\" \n",p,p->thread,PROCESS_NAME_STRING(p));
		}
	}
#endif

	while (1)
	{
		process_run();

		watchdog_periodic();

	}
	return 0;
}
