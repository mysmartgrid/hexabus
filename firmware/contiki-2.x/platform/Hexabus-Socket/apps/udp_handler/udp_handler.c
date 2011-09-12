/*   Copyright (c) 2010 Mathias Dalheimer
 *  All rights reserved.
 *
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the copyright holders nor the names of
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *Author: 	Günter Hildebrandt <guenter.hildebrandt@esk.fraunhofer.de>
 *			Mathias Dalheimer <>
 */

#include "udp_handler.h"
#include <string.h>
#include <stdbool.h>
#include <avr/wdt.h>
#include "dev/watchdog.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "webserver-nogui.h"
#include "httpd-cgi.h"
#include "dev/leds.h"


#include "metering.h"
#include "relay.h"
#include "eeprom_variables.h"


#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define UDP_DATA_LEN 120
#define UDP_IP_BUF   ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])
#define HEXABUS_PORT 61616

static struct uip_udp_conn *udpconn;
static struct etimer udp_periodic_timer;
static struct ctimer lost_connection_timer;

/* Command definitions. */
#define ON                     0x10 // Socket on
#define OFF                    0x11 // Socket off
#define STATUS_REQUEST	       0x12  // Socket status
#define STATUS_REPLY           0x22
#define VALUE                  0x13 // metering value
#define VALUE_REPLY            0x23
#define RESET                  0xFF // reset all values
#define RESET_REPLY			   0x2F
#define SET_DEFAULT            0xDE // set default values
#define HEARTBEAT_ACK	       0xAC
#define HEARTBEAT              0xBE
#define UPDATE_SERVER          0x1B


#define HEXABUS_HEADER "HEXABUS"
struct hexabusmsg_t {
	char header[7]; //header = HEXABUS
	uint8_t source; //source of the message (1: Server, 2: Socket)
	uint16_t command;
};
static bool heartbeat_ipaddr_set = false;
static uip_ipaddr_t heartbeat_ipaddr;
static unsigned char heartbeat_no_ack;
#define	MAX_HEARTBEAT_RETRIES	5 //maximum failed heartbeats before changing to default mode

PROCESS(udp_handler_process, "HEXABUS Socket UDP handler Process");
AUTOSTART_PROCESSES(&udp_handler_process);

/*---------------------------------------------------------------------------*/

static void
pollhandler(void) {
	PRINTF("----Socket_UDP_handler: Process polled\r\n");
}

static void
exithandler(void) {
	PRINTF("----Socket_UDP_handler: Process exits.\r\n");
}

/* flashing red and green led indicates lost connection to server */
static void lost_connection_leds(void) {
	if(heartbeat_no_ack > MAX_HEARTBEAT_RETRIES) {
	leds_toggle(LEDS_ALL);
	ctimer_reset(&lost_connection_timer);
	}
	else {
		relay_leds();
	}
}

/*---------------------------------------------------------------------------*/

static void
make_message(char* buf, uint16_t command, uint16_t value)
{
	struct hexabusmsg_t *hexabuscmd;
	hexabuscmd = (struct hexabusmsg_t *)buf;
	memset(hexabuscmd, 0, sizeof(struct hexabusmsg_t));
	strncpy(hexabuscmd->header, HEXABUS_HEADER, sizeof(hexabuscmd->header));
	hexabuscmd->source = 2;
	hexabuscmd->command = uip_htons(command);
	uint16_t *response;
	response = (uint16_t*) (hexabuscmd + 1);
	*response = uip_htons(value);
	PRINTF("udp_handler: Responding with message: ");
	PRINTF("%s%02x%02x%04d\n", HEXABUS_HEADER, 2, command, value);
}

static void
udphandler(process_event_t ev, process_data_t data)
{
	//static int seq_id;
	char buf[UDP_DATA_LEN];
	struct hexabusmsg_t *hexabuscmd;
	if (ev == tcpip_event) {
		if(uip_newdata()) {
			PRINTF("udp_handler: received '%d' bytes from ", uip_datalen());
			PRINT6ADDR(&UDP_IP_BUF->srcipaddr);
			PRINTF("\r\n");
			//      if (uip_datalen() == sizeof(hexabuscmd)) {
			hexabuscmd = (struct hexabusmsg_t *)uip_appdata;
			if ( !strncmp(hexabuscmd->header, HEXABUS_HEADER, sizeof(hexabuscmd->header)) && hexabuscmd->source == 1) {
				switch(uip_ntohs(hexabuscmd->command)) {
				case VALUE:
					//it copies the source address of the udp packet into the remote peer address, to which the response is going to be sent.
					uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr);
					udpconn->rport = UIP_HTONS(HEXABUS_PORT);
					PRINTF("udp_handler: Sending metering value.\r\n");
					make_message(buf, VALUE_REPLY, metering_get_power());
					uip_udp_packet_send(udpconn, buf, sizeof(struct hexabusmsg_t) + sizeof(uint16_t));
					break;
				case STATUS_REQUEST:
					PRINTF("udp_handler: Sending status.\r\n");
					uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr);
					udpconn->rport = UIP_HTONS(HEXABUS_PORT);
					make_message(buf, STATUS_REPLY,!relay_get_state());
					uip_udp_packet_send(udpconn, buf, sizeof(struct hexabusmsg_t) + sizeof(uint16_t));
					break;
				case ON:
					uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr);
					udpconn->rport = UIP_HTONS(HEXABUS_PORT);
					PRINTF("udp_handler: Switch ON.\r\n");
					relay_off();
					make_message(buf, STATUS_REPLY,!relay_get_state());
					uip_udp_packet_send(udpconn, buf, sizeof(struct hexabusmsg_t) + sizeof(uint16_t));
					break;
				case OFF:
					uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr);
					udpconn->rport = UIP_HTONS(HEXABUS_PORT);
					PRINTF("udp_handler: Switch OFF.\r\n");
					relay_on();
					make_message(buf, STATUS_REPLY,!relay_get_state());
					uip_udp_packet_send(udpconn, buf, sizeof(struct hexabusmsg_t) + sizeof(uint16_t));
					break;
				case RESET:
					uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr);
					udpconn->rport = UIP_HTONS(HEXABUS_PORT);
					PRINTF("udp_handler: RESET\r\n");
					make_message(buf, RESET_REPLY, 0);
					uip_udp_packet_send(udpconn, buf, sizeof(struct hexabusmsg_t));
					/* Execute reset via the watchdog */
					watchdog_reboot();
					break;
				case SET_DEFAULT:
					PRINTF("udp_handler: SET_DEFAULT\r\n");
					cli();
					eeprom_write_word((uint16_t *)EE_PAN_ID, 0xABCD); //set default pan id
					eeprom_write_byte((uint8_t *)EE_METERING_CAL_FLAG, 0xFF);// enable metering calibration
					uint8_t dns_name[] = {0x53, 0x6f, 0x63, 0x6b, 0x65, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Socket is the default dns name
					eeprom_write_block(dns_name, (void *)EE_DOMAIN_NAME, EE_DOMAIN_NAME_SIZE);
					/* Execute reset via the watchdog */
					watchdog_reboot();
					break;
				case HEARTBEAT_ACK:
					PRINTF("udp_handler: Heartbeat ACK received.\r\n");
					heartbeat_no_ack = 0;
					break;
				case UPDATE_SERVER:
					PRINTF("udp_handler: Setting Heartbeat destination address to: ");
					PRINT6ADDR(&UDP_IP_BUF->srcipaddr);
					PRINTF("\r\n");
					uip_ipaddr_copy(&heartbeat_ipaddr, &UDP_IP_BUF->srcipaddr);
					heartbeat_ipaddr_set = true;
					etimer_set(&udp_periodic_timer, 1); //reply with an heartbeat
					break;
				default:
					PRINTF("udp_handler: Unknown command received, ignoring.\r\n");
					break;
				}
			} else {
				PRINTF("udp_handler: Discarding message which is wrong or not for me!\r\n");
			}
			/* Restore server connection to allow data from any node */
			memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
			udpconn->rport = 0;
		}
	} else {//timer event
		if (heartbeat_ipaddr_set)
		{
			if(heartbeat_no_ack++ > MAX_HEARTBEAT_RETRIES)
			{
				relay_default(); //TODO write separate lost_connection function
				PRINTF("udp_handler: Lost connection to server, switching to default state!!!\r\n");
				ctimer_set(&lost_connection_timer, CLOCK_SECOND/2,(void *)(void *) lost_connection_leds,NULL);
			}

			uip_ipaddr_copy(&udpconn->ripaddr, &heartbeat_ipaddr); //set ip_addr
			udpconn->rport = UIP_HTONS(HEXABUS_PORT);   			//set port
			PRINTF("udp_handler: sending heartbeat to: ");
			PRINT6ADDR(&udpconn->ripaddr);
			PRINTF("\r\n");

			make_message(buf, HEARTBEAT, 0);
			uip_udp_packet_send(udpconn, buf, sizeof(struct hexabusmsg_t));

			/* Restore server connection to allow data from any node */
			memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
			udpconn->rport = 0;
		}
		etimer_set(&udp_periodic_timer, 60*CLOCK_SECOND);
	}
}

static void print_local_addresses(void) {
	int i;
	//  uip_netif_state state;

	PRINTF("\nAddresses [%u max]\n",UIP_DS6_ADDR_NB);
	for (i=0;i<UIP_DS6_ADDR_NB;i++) {
		if (uip_ds6_if.addr_list[i].isused) {
			PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
			PRINTF("\n");
		}
	}
	/*
  PRINTF("Current IPv6 addresses: \r\n");
  for(i = 0; i < UIP_CONF_NETIF_MAX_ADDRESSES; i++) {
    state = uip_netif_physical_if.addresses[i].state;
    if(state  != NOT_USED) { //== TENTATIVE || state == PREFERRED) {
      PRINT6ADDR(&uip_netif_physical_if.addresses[i].ipaddr);
      PRINTF("\n\r");
    }
    }*/
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udp_handler_process, ev, data) {
	uip_ipaddr_t ipaddr;
	PROCESS_POLLHANDLER(pollhandler());
	PROCESS_EXITHANDLER(exithandler());

	// see: http://senstools.gforge.inria.fr/doku.php?id=contiki:examples
	PROCESS_BEGIN();
	PRINTF("udp_handler: process startup.\r\n");
	// wait 3 second, in order to have the IP addresses well configured
	etimer_set(&udp_periodic_timer, CLOCK_CONF_SECOND*3);
	// wait until the timer has expired
	PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
	// Define Address of the server that receives our heartbeats.
	// TODO: Make this dynamic
#ifdef UDP_ADDR_A
	uip_ip6addr(&ipaddr,
			UDP_ADDR_A,UDP_ADDR_B,UDP_ADDR_C,UDP_ADDR_D,
			UDP_ADDR_E,UDP_ADDR_F,UDP_ADDR_G,UDP_ADDR_H);
#else /* UDP_ADDR_A */
	uip_ip6addr(&ipaddr,0xbbbb,0,0,0,0xd69a,0x20ff,0xfe07,0x7664);
#endif /* UDP_ADDR_A */

	udpconn = udp_new(NULL, UIP_HTONS(0), NULL);
	udp_bind(udpconn, UIP_HTONS(HEXABUS_PORT));
	// udp_attach(udpconn, NULL);

	PRINTF("udp_handler: Created connection with remote peer ");
	PRINT6ADDR(&udpconn->ripaddr);
	PRINTF("\r\nlocal/remote port %u/%u\r\n", uip_htons(udpconn->lport),uip_htons(udpconn->rport));

	print_local_addresses();
	etimer_set(&udp_periodic_timer, 60*CLOCK_SECOND);

	while(1){
		//   tcpip_poll_udp(udpconn);
		PROCESS_WAIT_EVENT();
		//    PROCESS_YIELD();
		udphandler(ev, data);
	}


	PROCESS_END();
}
