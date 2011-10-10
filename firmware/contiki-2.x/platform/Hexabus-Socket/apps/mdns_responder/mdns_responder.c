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


#include "mdns_responder.h"
#include <stdbool.h>
#include "sys/etimer.h" //contiki event timer library
#include "eeprom_variables.h"
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include <string.h>

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

#define UIP_IP_BUF ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define MDNS_NAME_SIZE 	(EE_DOMAIN_NAME_SIZE+10)
#define MDNS_PORT       5353
#define MDNS_TTL        120ULL  //120 seconds is the default time

static char 	domain_name[MDNS_NAME_SIZE];
static uint8_t 	domain_name_size;
static struct 	uip_udp_conn *mdns_conn = NULL;
static bool 	mdns_probe_state; // Checks to see if the probing phase is still going on
static bool 	domain_name_used; // Used in the probing phase: tells if someone else already uses the name
static struct etimer mdns_timer;
static uint8_t 	counter;      //Used in adding suffixes after the domain name, in case a specific name is already in use
static int 		number_of_tries; // Used in the probing. It shouldn't be greater than 3
// The socket is waiting for 3 ACK-s that no one is using its name, before it announces it

#define DNS_TYPE_A		(1)
#define DNS_TYPE_CNAME	(5)
#define DNS_TYPE_PTR	(12)
#define DNS_TYPE_MX		(15)
#define DNS_TYPE_TXT	(16)
#define DNS_TYPE_AAAA	(28)
#define DNS_TYPE_SRV	(33)
#define DNS_TYPE_ANY	(255) // Used in the probing

#define DNS_CLASS_IN	(1)
#define DNS_CACHE_FLUSH (1L << 15)

#if UIP_CONF_IPV6
static const uip_ipaddr_t resolv_mdns_addr = {
		.u8 = {
				0xff, 0x02, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xfb,
		}
};
#else
static const uip_ipaddr_t resolv_mdns_addr = {
		.u8 = { 224, 0, 0, 251 }
};
#endif

/** \internal The DNS message header. */
struct dns_hdr {
	u16_t id;
	u8_t flags1, flags2;
#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03
	u16_t numquestions;
	u16_t numanswers;
	u16_t numauthrr;
	u16_t numextrarr;
};

/** \internal The DNS answer message structure. */
struct dns_answer {
	/* DNS answer record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
	u16_t type;
	u16_t class;
	u16_t ttl[2];
	u16_t len;
	uip_ipaddr_t ipaddr;
};

/** \internal The DNS question message structure. */
struct dns_question {
	/* DNS question record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
	u16_t type;
	u16_t class;
};


void mdns_response(void);
void mdns_make_domain_name(uint8_t);
void mdns_probe_query();
void mdns_probe();
PROCESS(mdns_responder_process, "mdns_responder_process");

/*--------------------------------------------------------------------------*/


uint8_t
get_name_length(char *query)
{
	uint8_t len = 0;
	while(*(query++) != 0) {
		len++;
	} ;

	return len;
}

/*---------------------------------------------------------------------------*/
// Process the incoming data
void
mdns_handler(process_event_t ev, process_data_t data)
{
	struct dns_hdr *hdr;
	char *questionptr;
	static u8_t nquestions;
	static u8_t nanswers;

	if(uip_newdata()) {
		((char *)uip_appdata)[uip_datalen()] = 0;
		hdr = (struct dns_hdr *)uip_appdata;

		// The pointer for the query itself
		questionptr = (char*)hdr+sizeof(*hdr);

		uint8_t opcode_flag;
		opcode_flag = (hdr->flags1 & 0xF8);

		PRINTF("mDNS responder: received %d bytes from ", uip_datalen());
		PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
		PRINTF("\n");

		if ((opcode_flag & 0x78) == DNS_FLAG1_OPCODE_STANDARD){

			nquestions = (u8_t)uip_htons(hdr->numquestions);
			nanswers = (u8_t)uip_htons(hdr->numanswers);

			while (nquestions > 0){
				if(*questionptr & 0xc0) {
					/* Compressed name. */
					questionptr +=2;
				} else {
					/* Not compressed name. */
					uint8_t length = get_name_length(questionptr);

					if(length <= MDNS_NAME_SIZE)
					{
						/* compare domain_name with mDNS query name */
						unsigned int len = length-1;
						PRINTF("mDNS responder: queried domain name is: %s\n", questionptr);
						char www_str[4];
						sprintf(www_str, "%cwww",0x03);
						if  (!strncasecmp(questionptr, www_str, 4))
							questionptr+=4;
						if (!strncasecmp(questionptr, domain_name, len)){
							PRINTF("mDNS responder: queried domain name is our domain name.\n");
							mdns_response();
						}
					}
					// Move to the next question, but first skip the type and class flags: each 2 bytes long
					questionptr += length + 4;

				}
				nquestions--;
			}

			if (mdns_probe_state == true && opcode_flag == DNS_FLAG1_RESPONSE){

				while(nanswers>0){
					PRINTF("mDNS responder: received response.\n");
					if(*questionptr & 0xc0) {
						/* Compressed name.*/
						questionptr +=2;
					} else {
						/* Not compressed name.*/
						uint8_t len = get_name_length(questionptr);

						if(len == domain_name_size)
						{
							if (!strncasecmp(questionptr, domain_name, len)) {
								PRINTF("mDNS responder: probed name %s is already in use!\n", domain_name);
								domain_name_used = true;
							}
						}
					}
					nanswers--;
				}
			}
		}
	}
}
	/*-------------------------------------------------------------------*/

	void mdns_probe(void){
		if (domain_name_used){
			counter++; // Append new suffix to the hostname of the socket
			mdns_make_domain_name(counter);
			PRINTF("mDNS responder: domain name already in use, probing with new domain name: %s\n", domain_name);
			domain_name_used = false;
			number_of_tries = 0; // Reset the number of tries counter
		}
		else if (number_of_tries < 3){
			number_of_tries++;
			mdns_make_domain_name(counter);
			PRINTF("mDNS responder: probing domain name: %s\n", domain_name);
			mdns_probe_query();
		}
		else {
			// send announcement with the current name
			mdns_probe_state = false;
			PRINTF("mDNS responder: probing finished, announced domain name is: %s\n", domain_name);
			mdns_response();
		}
	}
	/*----------------------------------------------------------------------*/

	// Sends the probe query during the probing phase

	void mdns_probe_query()
	{

		struct dns_hdr *hdr;
		char *my_name, *response;
		struct dns_question *qst;
		uint8_t i;
		char buf[sizeof(struct dns_hdr) + domain_name_size + 1 + sizeof(struct dns_question)];

		response = buf;
		hdr = (struct dns_hdr *)buf;
		memset(hdr, 0, sizeof(struct dns_hdr));


		hdr->flags1 = DNS_FLAG1_OPCODE_STANDARD;
		hdr->numquestions = UIP_HTONS(1);

		//Name Field
		my_name = (char*) (hdr + 1);
		for(i=0; i < domain_name_size; i++) {
			*(my_name++) = domain_name[i];
		}
		*my_name=0; //ends with 0x00

		qst = (struct dns_question *) (my_name + 1);
		memset(qst, 0, sizeof(struct dns_question));


		qst->type = UIP_HTONS(DNS_TYPE_ANY);
		qst->class = UIP_HTONS(DNS_CLASS_IN);

		uip_udp_packet_sendto(
				mdns_conn,
				response,
				(sizeof(buf)),
				&resolv_mdns_addr,
				UIP_HTONS(MDNS_PORT)
		);
	}

	/*-----------------------------------------------------------------------------------*/

	// Creates the response
	void mdns_response(void)
	{
		struct dns_hdr *hdr;
		char *my_name, *response;
		struct dns_answer *ans;
		uint8_t i;
		char buf[sizeof(struct dns_hdr) + domain_name_size + 1 + sizeof(struct dns_answer)];

		response = (char*) buf;
		hdr = (struct dns_hdr *)buf;
		memset(hdr, 0, sizeof(struct dns_hdr));

		hdr->flags1 = DNS_FLAG1_RESPONSE;
		hdr->numanswers = UIP_HTONS(1);

		//Name Field
		my_name = (char*) (hdr + 1);
		//*(my_name++)=0x06; // starts length of first name
		for(i=0; i < domain_name_size; i++) {
			*(my_name++) = domain_name[i];
		}
		*my_name=0; //ends with 0x00

		ans = (struct dns_answer *) (my_name + 1);
		memset(ans, 0, sizeof(struct dns_answer));

#if UIP_CONF_IPV6
		ans->type = UIP_HTONS(DNS_TYPE_AAAA);
#else
		ans->type = UIP_HTONS(DNS_TYPE_A);
#endif
		ans->class = UIP_HTONS(DNS_CACHE_FLUSH | DNS_CLASS_IN);
		ans->ttl[1] = UIP_HTONS(MDNS_TTL);
		ans->ttl[0] = UIP_HTONS(MDNS_TTL >> 16);
		ans->len = UIP_HTONS(sizeof(uip_ipaddr_t));

		//mDNS-Answer ends with the IP-Address
		for (i=0;i<UIP_DS6_ADDR_NB;i++) {
			if (uip_ds6_if.addr_list[i].isused) {
				uip_ipaddr_copy(&ans->ipaddr, &uip_ds6_if.addr_list[i].ipaddr);
				break;
			}
		}
		PRINTF("mDNS responder: responding with ip address:");
		PRINT6ADDR(&ans->ipaddr);
		PRINTF("\n");
		uip_udp_packet_sendto(
				mdns_conn,
				response,
				(sizeof(buf)),
				&resolv_mdns_addr,
				uip_htons(MDNS_PORT)
		);
	}

	/*---------------------------------------------------------------------------*/
	PROCESS_THREAD(mdns_responder_process, ev, data)
	{
		PROCESS_BEGIN();
		PRINTF("mDNS responder: started\n");

		if(!mdns_conn) {
			mdns_conn = udp_new(NULL,0,NULL);
			uip_udp_bind(mdns_conn,UIP_HTONS(MDNS_PORT));
			mdns_conn->rport = 0;
			// The timer should be set in the PROCESS function itself
			etimer_set(&mdns_timer, CLOCK_SECOND * 10);
		}
		else {
			etimer_set(&mdns_timer, 1);
		}

		while(1) {
			PROCESS_YIELD();
			if(ev == tcpip_event) {
				PRINTF("mDNS responder: tcpip_event\n");
				mdns_handler(ev, data);
			}
			else if(etimer_expired(&mdns_timer)) {
				PRINTF("mDNS responder: timer expired\n");
				mdns_probe();
				if (mdns_probe_state == true)
					etimer_set(&mdns_timer, CLOCK_SECOND / 6);
			}
		}

		PROCESS_END();
	}
	/*---------------------------------------------------------------------------*/
	// Appends a suffix to the domain name
	void mdns_make_domain_name(uint8_t extension)
	{
		char tmp[MDNS_NAME_SIZE];
		eeprom_read_block(tmp,(const void*) EE_DOMAIN_NAME, EE_DOMAIN_NAME_SIZE);
		if (extension == 0){
			domain_name_size = get_name_length(tmp);
			sprintf(domain_name, "%c%s%clocal", domain_name_size, tmp, 0x05); // 0x05 == ENQ (enquiry = Abfage) in hexadecimal
			domain_name_size = 1 + domain_name_size + 1 + 5; //TODO dynamic suffix // zeichen+domain_name+null+".local"
		} else {
			sprintf(tmp, "%s%d", tmp, extension);
			domain_name_size = get_name_length(tmp);
			sprintf(domain_name, "%c%s%clocal", domain_name_size, tmp, 0x05);
			domain_name_size = 1 + domain_name_size + 1 + 5; //TODO dynamic suffix
		}
	}

	/*----------------------------------------------------------------------------*/
	// Init function
	void
	mdns_responder_init(void)
	{
		PRINTF("mDNS responder: init\n");
		mdns_probe_state = true;
		domain_name_used=false;
		number_of_tries=0;
		counter=0;
		process_start(&mdns_responder_process, NULL);

	}

	void
	mdns_responder_set_domain_name(char *name, uint8_t len)
	{
		char tmp[EE_DOMAIN_NAME_SIZE];
		memset(tmp,0,EE_DOMAIN_NAME_SIZE);
		memcpy(tmp,name,len);
		eeprom_write_block(tmp, (void*) EE_DOMAIN_NAME, EE_DOMAIN_NAME_SIZE);
		process_exit(&mdns_responder_process);
		mdns_responder_init();
	}



