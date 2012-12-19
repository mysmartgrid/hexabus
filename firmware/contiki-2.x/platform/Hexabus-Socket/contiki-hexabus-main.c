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

#include "loader/symbols-def.h"
#include "loader/symtab.h"

#if RF230BB        //radio driver using contiki core mac
#include "radio/rf230bb/rf230bb.h"
#include "net/mac/frame802154.h"
#include "net/mac/framer-802154.h"
#include "net/sicslowpan.h"

#elif RF212BB        //radio driver using contiki core mac
#include "radio/rf212bb/rf212bb.h"
#include "net/mac/frame802154.h"
#include "net/mac/framer-802154.h"
#include "net/sicslowpan.h"

#else                 //radio driver using Atmel/Cisco 802.15.4'ish MAC
#include <stdbool.h>
#include "mac.h"
#include "sicslowmac.h"
#include "sicslowpan.h"
#include "ieee-15-4-manager.h"
#endif /*RF230BB*/

#include "contiki.h"
#include "contiki-net.h"
#include "contiki-lib.h"

#include "dev/rs232.h"
#include "dev/serial-line.h"
#include "dev/slip.h"

#if WEBSERVER
#include "httpd-fs.h"
#include "httpd-cgi.h"
#include "webserver-nogui.h"
#endif

#ifdef COFFEE_FILES
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"
#endif

#if UIP_CONF_ROUTER&&0
#include "net/routing/rimeroute.h"
#include "net/rime/rime-udp.h"
#endif

#include "net/rime.h"


//HEXABUS includes
#include "button.h"
#include "metering.h"
#include "relay.h"
#include "eeprom_variables.h"
#include "udp_handler.h"
#include "mdns_responder.h"
#include "state_machine.h"
#include "hexabus_app_bootstrap.h"

// optional HEXABUS apps
#if VALUE_BROADCAST_ENABLE
#include "value_broadcast.h"
#endif
#if DATETIME_SERVICE_ENABLE
#include "datetime_service.h"
#endif
#if MEMORY_DEBUGGER_ENABLE
#include "memory_debugger.h"
#endif


uint8_t nSensors = 0; //number of found temperature sensors

uint8_t forwarding_enabled; //global variable for forwarding
uint8_t encryption_enabled = 1; //global variable for AES encryption
/*-------------------------------------------------------------------------*/
/*----------------------Configuration of the .elf file---------------------*/
#if 1
/* The proper way to set the signature is */
#include <avr/signature.h>
#else
/* Older avr-gcc's may not define the needed SIGNATURE bytes. Do it manually if you get an error */
typedef struct {unsigned char B2;unsigned char B1;unsigned char B0;} __signature_t;
#define SIGNATURE __signature_t __signature __attribute__((section (".signature")))
SIGNATURE = {
/* Older AVR-GCCs may not define the SIGNATURE_n bytes so use explicit 1284p values */
		.B2 = 0x05,//SIGNATURE_2,
		.B1 = 0x97,//SIGNATURE_1,
		.B0 = 0x1E,//SIGNATURE_0,
		};
#endif

FUSES = {.low = 0xE2, .high = 0x90, .extended = 0xFF,};



/*----------------------Configuration of EEPROM---------------------------*/
/* Use existing EEPROM if it passes the integrity test, else reinitialize with build values */

volatile uint8_t ee_mem[EESIZE] EEMEM =
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



bool get_mac_from_eeprom(uint8_t* macptr) {
	eeprom_read_block ((void *)macptr, (const void *)EE_MAC_ADDR, EE_MAC_ADDR_SIZE);
	return true;
}

uint16_t get_panid_from_eeprom(void) {
	return eeprom_read_word ((const void *)EE_PAN_ID);
}

uint16_t get_panaddr_from_eeprom(void) {
	return eeprom_read_word ((const void *)EE_PAN_ADDR);
}

uint8_t get_relay_default_from_eeprom(void) {
	return eeprom_read_byte ((const void *)EE_RELAY_DEFAULT);
}

uint8_t get_forwarding_from_eeprom(void) {
	return eeprom_read_byte ((const void *)EE_FORWARDING);
}

void get_aes128key_from_eeprom(uint8_t keyptr[16]) {
	eeprom_read_block ((void *)keyptr, (const void *)EE_ENCRYPTION_KEY, EE_ENCRYPTION_KEY_SIZE);
}

void set_forwarding_to_eeprom(uint8_t val) {
	 eeprom_write_byte ((uint8_t *)EE_FORWARDING, val);
	 forwarding_enabled = val;
}

/*-------------------------Low level initialization------------------------*/
/*------Done in a subroutine to keep main routine stack usage small--------*/
void initialize(void)
{
  watchdog_init();
  watchdog_start();

  init_lowlevel();

  clock_init();

  forwarding_enabled = get_forwarding_from_eeprom();


#if ANNOUNCE_BOOT
  PRINTF("\n*******Booting %s*******\n",CONTIKI_VERSION_STRING);
#endif

/* rtimers needed for radio cycling */
  rtimer_init();

 /* Initialize process subsystem */
  process_init();
 /* etimers must be started before ctimer_init */
  process_start(&etimer_process, NULL);

#if RF230BB || RF212BB

  ctimer_init();
  /* Start radio and radio receive process */

    NETSTACK_RADIO.init();
  /* Set addresses BEFORE starting tcpip process */

  rimeaddr_t addr;
  memset(&addr, 0, sizeof(rimeaddr_t));
  get_mac_from_eeprom(addr.u8);
 
#if UIP_CONF_IPV6 
  memcpy(&uip_lladdr.addr, &addr.u8, 8);
#endif  
#if RF230BB
  rf230_set_pan_addr(
	get_panid_from_eeprom(),
	get_panaddr_from_eeprom(),
	(uint8_t *)&addr.u8
  );
  rf230_set_channel(CHANNEL_802_15_4);

#elif RF212BB
  rf212_set_pan_addr(
	get_panid_from_eeprom(),
 	get_panaddr_from_eeprom(),
 	(uint8_t *)&addr.u8
   );
#endif

	extern uint16_t mac_dst_pan_id;
	extern uint16_t mac_src_pan_id;
	//set pan_id for frame creation
	mac_dst_pan_id = get_panid_from_eeprom();
	mac_src_pan_id = mac_dst_pan_id;

  rimeaddr_set_node_addr(&addr); 

  PRINTFD("MAC address %x:%x:%x:%x:%x:%x:%x:%x\n",addr.u8[0],addr.u8[1],addr.u8[2],addr.u8[3],addr.u8[4],addr.u8[5],addr.u8[6],addr.u8[7]);

  /* Initialize stack protocols */
  queuebuf_init();
  NETSTACK_RDC.init();
  NETSTACK_MAC.init();
  NETSTACK_NETWORK.init();

#if ANNOUNCE_BOOT
 #if RF230BB
  PRINTF("%s %s, channel %u",NETSTACK_MAC.name, NETSTACK_RDC.name, rf230_get_channel());
 #elif RF212BB
  PRINTF("%s %s, channel %u",NETSTACK_MAC.name, NETSTACK_RDC.name, rf212_get_channel());
 #endif /* RF230BB */
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

// rime_init(rime_udp_init(NULL));
// uip_router_register(&rimeroute);

  process_start(&tcpip_process, NULL);

#else
/* Original RF230 combined mac/radio driver */
/* mac process must be started before tcpip process! */
  process_start(&mac_process, NULL);
  process_start(&tcpip_process, NULL);
#endif /*RF230BB || RF212BB*/

#if WEBSERVER
  process_start(&webserver_nogui_process, NULL);
#endif

#if MEMORY_DEBUGGER_ENABLE 
  memory_debugger_init();
  /* periodically print memory usage */
  process_start(&memory_debugger_process, NULL);
#endif

  /* Handler for HEXABUS UDP Packets */
  process_start(&udp_handler_process, NULL);

  /* Process for periodic sending of HEXABUS data */
#if VALUE_BROADCAST_ENABLE
#if VALUE_BROADCAST_AUTO_INTERVAL
  process_start(&value_broadcast_process, NULL);
#else
  init_value_broadcast();
#endif
#endif

  /* process handling received HEXABUS broadcasts */
#if STATE_MACHINE_ENABLE
  process_start(&state_machine_process, NULL);
#endif

  mdns_responder_init();

  /* Datetime service*/
#if DATETIME_SERVICE_ENABLE
  process_start(&datetime_service_process, NULL);
#endif

  hexabus_bootstrap_init_apps();
  hexabus_bootstrap_start_processes();

  /* Autostart other processes */
  autostart_start(autostart_processes);

  /*---If using coffee file system create initial web content if necessary---*/
#if COFFEE_FILES
  int fa = cfs_open( "/index.html", CFS_READ);
  if (fa<0) {     //Make some default web content
    PRINTF("No index.html file found, creating upload.html!\n");
    PRINTF("Formatting FLASH file system for coffee...");
    cfs_coffee_format();
    PRINTF("Done!\n");
    fa = cfs_open( "/index.html", CFS_WRITE);
    int r = cfs_write(fa, &"It works!", 9);
    if (r<0) PRINTF("Can''t create /index.html!\n");
    cfs_close(fa);
//  fa = cfs_open("upload.html"), CFW_WRITE);
// <html><body><form action="upload.html" enctype="multipart/form-data" method="post"><input name="userfile" type="file" size="50" /><input value="Upload" type="submit" /></form></body></html>
  }
#endif /* COFFEE_FILES */

/* Add addresses for testing */
#if 0
{  
  uip_ip6addr_t ipaddr;
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
//  uip_ds6_prefix_add(&ipaddr,64,0);
}
#endif

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
	eeprom_read_block(buf, (const void*) EE_DOMAIN_NAME, EE_DOMAIN_NAME_SIZE);
	buf[EE_DOMAIN_NAME_SIZE] = 0;
	size=httpd_fs_get_size();
#ifndef COFFEE_FILES
   PRINTF(".%s online with fixed %u byte web content\n",buf,size);
#elif COFFEE_FILES==1
   PRINTF(".%s online with static %u byte EEPROM file system\n",buf,size);
#elif COFFEE_FILES==2
   PRINTF(".%s online with dynamic %u KB EEPROM file system\n",buf,size>>10);
#elif COFFEE_FILES==3
   PRINTF(".%s online with static %u byte program memory file system\n",buf,size);
#elif COFFEE_FILES==4
   PRINTF(".%s online with dynamic %u KB program memory file system\n",buf,size>>10);
#endif /* COFFEE_FILES */

#else
   PRINTF("Online\n");
#endif /* WEBSERVER */

#endif /* ANNOUNCE_BOOT */
}

#if RF230BB
extern char rf230_interrupt_flag, rf230processflag;
#elif RF212BB
extern char rf212_interrupt_flag, rf212processflag; // this is still not implemented
#endif

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

