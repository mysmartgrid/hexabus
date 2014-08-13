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
#define ANNOUNCE_BOOT 1    //adds about 600 bytes to program size
#define PRINTF(FORMAT,args...) printf_P(PSTR(FORMAT),##args)

#define DEBUG 1
#if DEBUG
#define PRINTFD(FORMAT,args...) printf_P(PSTR(FORMAT),##args)
#else
#define PRINTFD(...)
#endif

#include <hexabus_config.h>

#include <avr/pgmspace.h>
#include <avr/fuse.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include <string.h>
#include <dev/watchdog.h>

#include "radio/rf230bb/rf230bb.h"
#include "net/sicslowpan.h"

#include "contiki.h"

#include "lib/random.h"

#if WEBSERVER
#include "httpd-fs.h"
#include "httpd-cgi.h"
#include "webserver-nogui.h"
#endif

#include "net/rime.h"


//HEXABUS includes
#include "button_handlers.h"
#include "contiki-hexabus.h"
#include "button.h"
#include "hexabus_app_bootstrap.h"
#include "nvm.h"

#include "health.h"

uint8_t nSensors = 0; //number of found temperature sensors

uint8_t encryption_enabled = 1; //global variable for AES encryption
/*-------------------------------------------------------------------------*/
/*----------------------Configuration of the .elf file---------------------*/
#include <avr/signature.h>

FUSES = {.low = 0xE2, .high = 0x90, .extended = 0xFF,};



/*----------------------Configuration of EEPROM---------------------------*/
/* Use existing EEPROM if it passes the integrity test, else reinitialize with build values */

volatile uint8_t ee_mem[EEP_SIZE] EEMEM =
{
	0x00, // ee_dummy
	0x02, 0x11, 0x22, 0xff, 0xfe, 0x33, 0x44, 0x11, // ee_mac_addr
	0x00, 0x00, // ee_pan_addr
	0xCD, 0xAB, // ee_pan_id
	0x00, 		//ee_bootloader_flag
	0x00, 0x00,	//ee_bootloader_crc
	0x01,		//ee_first_run
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //ee_encryption_key
	0x53, 0x6f, 0x63, 0x6b, 0x65, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // ee_domain_name (Socket)
	0xCD, 0x78, //ee_metering_ref (247,4 * 125) = 30925;
	0x64, 0x00, //ee_metering_cal_load (100 W)
	0xFF, 		//ee_metering_cal_flag (0xFF: calibration is unlocked, any other value: locked)
	0x00, 		//ee_relay_default (0x00: off, 0x01: on)
	0x00,		 //ee_forwarding (0x00: off, 0x01: forward all incoming traffic);
};



void get_mac_from_eeprom(uint8_t* macptr)
{
	nvm_read_block(mac_addr, macptr, nvm_size(mac_addr));
}

uint16_t get_panid_from_eeprom(void)
{
	return nvm_read_u16(pan_id);
}

uint16_t get_panaddr_from_eeprom(void)
{
	return nvm_read_u16(pan_addr);
}

uint8_t get_relay_default_from_eeprom(void)
{
	return nvm_read_u8(relay_default);
}

void get_aes128key_from_eeprom(uint8_t keyptr[16])
{
	nvm_read_block(encryption_key, keyptr, 16);
}

/*-------------------------Low level initialization------------------------*/
/*------Done in a subroutine to keep main routine stack usage small--------*/
void initialize(void)
{
  watchdog_init();
  watchdog_start();
	button_handlers_init();

  init_lowlevel();

  clock_init();

#if ANNOUNCE_BOOT
  PRINTF("\n*******Booting %s*******\n",CONTIKI_VERSION_STRING);
#endif

	health_event = process_alloc_event();

/* rtimers needed for radio cycling */
  rtimer_init();

 /* Initialize process subsystem */
  process_init();
 /* etimers must be started before ctimer_init */
  process_start(&etimer_process, NULL);

  ctimer_init();
  /* Start radio and radio receive process */

  /* Initialize stack protocols */
  queuebuf_init();
  netstack_init();

  /* Set addresses BEFORE starting tcpip process */

  rimeaddr_t addr;
  memset(&addr, 0, sizeof(rimeaddr_t));
  get_mac_from_eeprom(addr.u8);

  memcpy(&uip_lladdr.addr, &addr.u8, 8);
  rf230_set_pan_addr(
	get_panid_from_eeprom(),
	get_panaddr_from_eeprom(),
	(uint8_t *)&addr.u8
  );
  rf230_set_channel(RF_CHANNEL);

	extern uint16_t mac_dst_pan_id;
	extern uint16_t mac_src_pan_id;
	//set pan_id for frame creation
	mac_dst_pan_id = get_panid_from_eeprom();
	mac_src_pan_id = mac_dst_pan_id;

  rimeaddr_set_node_addr(&addr); 

  PRINTFD("MAC address %x:%x:%x:%x:%x:%x:%x:%x\n",addr.u8[0],addr.u8[1],addr.u8[2],addr.u8[3],addr.u8[4],addr.u8[5],addr.u8[6],addr.u8[7]);

#if ANNOUNCE_BOOT
  PRINTF("%s %s, channel %u",NETSTACK_MAC.name, NETSTACK_RDC.name, rf230_get_channel());
  if (NETSTACK_RDC.channel_check_interval) {//function pointer is zero for sicslowmac
    unsigned short tmp;
    tmp=CLOCK_SECOND / (NETSTACK_RDC.channel_check_interval == 0 ? 1:\
                                   NETSTACK_RDC.channel_check_interval());
    if (tmp<65535) printf_P(PSTR(", check rate %u Hz"),tmp);
  }
  PRINTF("\n");

#if UIP_CONF_IPV6_RPL
  PRINTF("RPL Enabled\n");
#endif
#if UIP_CONF_ROUTER
  PRINTF("Routing Enabled\n");
#endif

#endif /* ANNOUNCE_BOOT */

  process_start(&tcpip_process, NULL);

  random_init((rf230_generate_random_byte() << 8) | rf230_generate_random_byte());

  hexabus_bootstrap_init_apps();
  hexabus_bootstrap_start_processes();

/*--------------------------Announce the configuration---------------------*/
#if ANNOUNCE_BOOT

#if WEBSERVER
  uint8_t i;
  char buf[80];
  unsigned int size;

  for (i=0;i<UIP_DS6_ADDR_NB;i++) {
	if (uip_ds6_if.addr_list[i].isused) {	  
	   httpd_cgi_sprint_ip6(uip_ds6_if.addr_list[i].ipaddr,buf);
       PRINTF("IPv6 Address: %s\n",buf);
	}
  }
	nvm_read_block(domain_name, buf, nvm_size(domain_name));
	buf[nvm_size(domain_name)] = 0;
	size=httpd_fs_get_size();
	PRINTF("%s online with fixed %u byte web content\n",buf,size);
#else
   PRINTF("Online\n");
#endif /* WEBSERVER */

#endif /* ANNOUNCE_BOOT */
}

/*-------------------------------------------------------------------------*/
/*------------------------- Main Scheduler loop----------------------------*/
/*-------------------------------------------------------------------------*/
int
main(void)
{
	initialize();

	while(1) {
		process_run();
		watchdog_periodic();
	}
  return 0;
}

/*---------------------------------------------------------------------------*/

void log_message(char *m1, char *m2)
{
  PRINTF("%s%s\n", m1, m2);
}

