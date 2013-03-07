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
#include <stdlib.h>
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "httpd-cgi.h"
#include "packet_builder.h"
#include "lib/crc16.h"
#include <util/delay.h>

#include "state_machine.h"
#include "datetime_service.h"
#include "endpoint_registry.h"
#include "hexabus_config.h"
#include "../../../../../../shared/hexabus_packet.h"

#if UDP_HANDLER_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define UDP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

static struct uip_udp_conn *udpconn;
static struct etimer udp_periodic_timer;

static uip_ipaddr_t hxb_group;

PROCESS(udp_handler_process, "HEXABUS Socket UDP handler Process");
AUTOSTART_PROCESSES(&udp_handler_process);

process_event_t sm_data_received_event;

/*---------------------------------------------------------------------------*/
static void
pollhandler(void) {
  PRINTF("----Socket_UDP_handler: Process polled\r\n");
}

static void
exithandler(void) {
  PRINTF("----Socket_UDP_handler: Process exits.\r\n");
}

void send_packet(const void* data, size_t length)
{
  uip_ipaddr_copy(&udpconn->ripaddr, &UDP_IP_BUF->srcipaddr); // reply to the IP from which the request came
  udpconn->rport = UDP_IP_BUF->srcport;
  udpconn->lport = UIP_HTONS(HXB_PORT);
  uip_udp_packet_send(udpconn, data, length);
  PRINTF("%d bytes sent.\r\n", length);

  /* Restore server connection to allow data from any node */
  memset(&udpconn->ripaddr, 0, sizeof(udpconn->ripaddr));
  udpconn->rport = 0;
}

static enum hxb_error_code check_crc(const union hxb_packet_any* packet)
{
	uint16_t data_len = 0;
	uint16_t crc = 0;
	switch ((enum hxb_packet_type) packet->header.type) {
		case HXB_PTYPE_ERROR:
			data_len = sizeof(packet->p_error);
			crc = packet->p_error.crc;
			break;

		case HXB_PTYPE_INFO:
		case HXB_PTYPE_WRITE:
			switch ((enum hxb_datatype) packet->value_header.datatype) {
				case HXB_DTYPE_BOOL:
				case HXB_DTYPE_UINT8:
					data_len = sizeof(packet->p_u8);
					crc = packet->p_u8.crc;
					break;

				case HXB_DTYPE_UINT32:
				case HXB_DTYPE_TIMESTAMP:
					data_len = sizeof(packet->p_u32);
					crc = packet->p_u32.crc;
					break;

				case HXB_DTYPE_FLOAT:
					data_len = sizeof(packet->p_float);
					crc = packet->p_float.crc;
					break;

				case HXB_DTYPE_128STRING:
					data_len = sizeof(packet->p_128string);
					crc = packet->p_128string.crc;
					break;

				case HXB_DTYPE_DATETIME:
					data_len = sizeof(packet->p_datetime);
					crc = packet->p_datetime.crc;
					break;

				case HXB_DTYPE_66BYTES:
					data_len = sizeof(packet->p_66bytes);
					crc = packet->p_66bytes.crc;
					break;

				case HXB_DTYPE_16BYTES:
					data_len = sizeof(packet->p_16bytes);
					crc = packet->p_16bytes.crc;
					break;

				case HXB_DTYPE_UNDEFINED:
				default:
					return HXB_ERR_MALFORMED_PACKET;
			}
			break;

		case HXB_PTYPE_EPINFO:
			data_len = sizeof(packet->p_128string);
			crc = packet->p_128string.crc;
			break;

		case HXB_PTYPE_QUERY:
		case HXB_PTYPE_EPQUERY:
			data_len = sizeof(packet->p_query);
			crc = packet->p_query.crc;
			break;

		default:
			return HXB_ERR_MALFORMED_PACKET;
	}

	if (uip_ntohs(crc) != crc16_data((unsigned char*) packet, data_len - 2, 0)) {
		PRINTF("CRC check failed\n");
		return HXB_ERR_CRCFAILED;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code extract_value(union hxb_packet_any* packet, struct hxb_value* value)
{
	value->datatype = packet->value_header.datatype;

	union {
		uint32_t u;
		float    f;
	} fconv;

	// CRC check and how big the actual value is depend on what type of packet we have.
	switch (value->datatype) {
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			value->v_u8 = packet->p_u8.value;
			break;

		case HXB_DTYPE_UINT32:
		case HXB_DTYPE_TIMESTAMP:
			value->v_u32 = uip_ntohl(packet->p_u32.value);
			break;

		case HXB_DTYPE_DATETIME:
			value->v_datetime = packet->p_datetime.value;
			value->v_datetime.year = uip_ntohs(value->v_datetime.year);
			break;

		case HXB_DTYPE_FLOAT:
			fconv.f = uip_ntohl(packet->p_float.value);
			fconv.u = uip_ntohl(fconv.u);
			value->v_float = fconv.f;
			break;

		case HXB_DTYPE_128STRING:
			value->v_string = packet->p_128string.value;
			break;

		case HXB_DTYPE_66BYTES:
			value->v_binary = packet->p_66bytes.value;
			break;

		case HXB_DTYPE_16BYTES:
			value->v_binary = packet->p_16bytes.value;
			break;

		case HXB_DTYPE_UNDEFINED:
		default:
			PRINTF("Packet of unknown datatype.\r\n");
			return HXB_ERR_DATATYPE;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code handle_write(union hxb_packet_any* packet)
{
	struct hxb_value value;

	uint32_t eid = uip_ntohl(packet->value_header.eid);

	enum hxb_error_code err;
	
	err = extract_value(packet, &value);
	if (err) {
		return err;
	}

	return endpoint_write(eid, &value);
}

static enum hxb_error_code handle_query(struct hxb_packet_query* query)
{
	uint32_t eid = uip_ntohl(query->eid);

	struct hxb_value value;
	enum hxb_error_code err;
	size_t len;

	union hxb_packet_any result;

	result.eid_header.type = HXB_PTYPE_INFO;
	result.eid_header.eid = eid;

	PRINTF("Received query for %lu\n", eid);

	// point v_binary and v_string to the appropriate buffer in the source packet.
	// that way we avoid allocating another buffer and unneeded copies
	value.v_string = result.p_128string.value;

	err = endpoint_read(eid, &value);
	if (err) {
		return err;
	}

	if (uip_ipaddr_cmp(&UDP_IP_BUF->destipaddr, &hxb_group)) {
		PRINTF("Group query!\r\n");
		clock_delay_usec(random_rand() >> 1); // wait for max 31ms
	}

	switch (value.datatype) {
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			result.p_u8.value = value.v_u8;
			packet_finalize_u8(&result.p_u8);
			len = sizeof(result.p_u8);
			break;

		case HXB_DTYPE_UINT32:
		case HXB_DTYPE_TIMESTAMP:
			result.p_u32.value = uip_htonl(value.v_u32);
			packet_finalize_u32(&result.p_u32);
			len = sizeof(result.p_u32);
			break;

		case HXB_DTYPE_DATETIME:
			result.p_datetime.value = value.v_datetime;
			packet_finalize_datetime(&result.p_datetime);
			len = sizeof(result.p_datetime);
			break;

		case HXB_DTYPE_FLOAT:
			result.p_float.value = value.v_float;
			packet_finalize_float(&result.p_float);
			len = sizeof(result.p_float);
			break;

		// these just work, because we pointed v_string and v_binary at the appropriate field in the
		// packet union.
		case HXB_DTYPE_128STRING:
			result.p_128string.value[HXB_STRING_PACKET_MAX_BUFFER_LENGTH] = 0;
			packet_finalize_128string(&result.p_128string);
			len = sizeof(result.p_128string);
			break;

		case HXB_DTYPE_66BYTES:
			packet_finalize_66bytes(&result.p_66bytes);
			len = sizeof(result.p_66bytes);
			break;

		case HXB_DTYPE_16BYTES:
			packet_finalize_16bytes(&result.p_16bytes);
			len = sizeof(result.p_16bytes);
			break;

		case HXB_DTYPE_UNDEFINED:
		default:
			return HXB_ERR_DATATYPE;
	}

	send_packet(&result, len);

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code handle_epquery(const struct hxb_packet_query* query)
{
	struct hxb_packet_128string result;

	result.type = HXB_PTYPE_EPINFO;
	result.eid = uip_ntohl(query->eid);
	result.datatype = endpoint_get_datatype(result.eid);
	if (result.datatype == HXB_DTYPE_UNDEFINED) {
		return HXB_ERR_UNKNOWNEID;
	}

	enum hxb_error_code err;

	err = endpoint_get_name(result.eid, result.value, HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
	result.value[HXB_STRING_PACKET_MAX_BUFFER_LENGTH] = '\0';
	if (err) {
		return err;
	}

	send_packet(&result, sizeof(result));
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code handle_info(union hxb_packet_any* packet)
{
	struct hxb_envelope envelope;
	enum hxb_error_code err;

	memcpy(envelope.source, &UDP_IP_BUF->srcipaddr, 16);
	envelope.eid = uip_ntohl(packet->eid_header.eid);


	err = extract_value(packet, &envelope.value);
	if (err) {
		return HXB_ERR_SUCCESS;
	}

	if (packet->value_header.datatype != HXB_DTYPE_DATETIME) {
#if STATE_MACHINE_ENABLE
		process_post_synch(PROCESS_BROADCAST, sm_data_received_event, &envelope);
#else
		PRINTF("Received Broadcast, but no handler for datatype.\r\n");
#endif
	} else {
#if DATETIME_SERVICE_ENABLE
		updateDatetime(&envelope);
#else
		PRINTF("Received Broadcast, but no handler for datatype.\r\n");
#endif
	}

	return HXB_ERR_SUCCESS;
}

static void
udphandler(process_event_t ev, process_data_t data)
{
  if (ev == tcpip_event) {
    if(uip_newdata()) {
      PRINTF("udp_handler: received '%d' bytes from ", uip_datalen());
      PRINT6ADDR(&UDP_IP_BUF->srcipaddr);
      PRINTF("\r\n");

			union hxb_packet_any* packet = (union hxb_packet_any*)uip_appdata;

      // check if it's a Hexabus packet
      if(strncmp(packet->header.magic, HXB_HEADER, 4)) {
        PRINTF("Received something, but it wasn't a Hexabus packet. Ignoring it.");
      } else {
				enum hxb_error_code err;

				// don't send error packets for broadcasts
				bool is_broadcast = packet->header.type == HXB_PTYPE_INFO
					&& uip_ipaddr_cmp(&UDP_IP_BUF->destipaddr, &hxb_group);

				err = check_crc(packet);
				if (err) {
					if (!is_broadcast && !(err & HXB_ERR_INTERNAL)) {
						struct hxb_packet_error err_packet = make_error_packet(err);
						send_packet(&err_packet, sizeof(err_packet));
					}
					return;
				}

				switch ((enum hxb_packet_type) packet->header.type) {
					case HXB_PTYPE_WRITE:
						err = handle_write(packet);
						break;

					case HXB_PTYPE_QUERY:
						err = handle_query(&packet->p_query);
						break;

					case HXB_PTYPE_EPQUERY:
						err = handle_epquery(&packet->p_query);
						break;

					case HXB_PTYPE_INFO:
						err = handle_info(packet);
						break;

					case HXB_PTYPE_ERROR:
					case HXB_PTYPE_EPINFO:
					default:
						PRINTF("packet of type %d received, but we do not know what to do with that (yet)\r\n", header->type);
						err = HXB_ERR_UNEXPECTED_PACKET;
						break;
				}

				if (err) {
					if (!is_broadcast && !(err & HXB_ERR_INTERNAL)) {
						struct hxb_packet_error err_packet = make_error_packet(err);
						send_packet(&err_packet, sizeof(err_packet));
					}
					return;
				}
      }
    }
  }
}

static void print_local_addresses(void) {
  PRINTF("\nAddresses [%u max]\n",UIP_DS6_ADDR_NB);
  int i;
  for (i=0;i<UIP_DS6_ADDR_NB;i++) {
    if (uip_ds6_if.addr_list[i].isused) {
      PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
      PRINTF("\n");
    }
  }
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udp_handler_process, ev, data) {
//  static uip_ipaddr_t ipaddr;
  PROCESS_POLLHANDLER(pollhandler());
  PROCESS_EXITHANDLER(exithandler());

  // see: http://senstools.gforge.inria.fr/doku.php?id=contiki:examples
  PROCESS_BEGIN();

  sm_data_received_event = process_alloc_event();

  PRINTF("udp_handler: process startup.\r\n");
  // wait 3 second, in order to have the IP addresses well configured
  etimer_set(&udp_periodic_timer, CLOCK_CONF_SECOND*3);
  // wait until the timer has expired
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

  // register the multicast address we want to listen on

  // this wrapper macro is needed to expand HXB_GROUP_RAW before uip_ip6addr, which is a macro
  // it's ugly as day, but it's the least ugly solution i found
  #define PARSER_WRAP(addr, ...) uip_ip6addr(addr, __VA_ARGS__)
  PARSER_WRAP(&hxb_group, HXB_GROUP_RAW);
  #undef PARSER_WRAP
  uip_ds6_maddr_add(&hxb_group);

  udpconn = udp_new(NULL, UIP_HTONS(0), NULL);
  udp_bind(udpconn, UIP_HTONS(HXB_PORT));
  // udp_attach(udpconn, NULL);

  PRINTF("udp_handler: Created connection with remote peer ");
  PRINT6ADDR(&udpconn->ripaddr);
  PRINTF("\r\nlocal/remote port %u/%u\r\n", uip_htons(udpconn->lport),uip_htons(udpconn->rport));

  print_local_addresses();
  etimer_set(&udp_periodic_timer, 60*CLOCK_SECOND);

  while(1) {
    //   tcpip_poll_udp(udpconn);
    PROCESS_WAIT_EVENT();
    udphandler(ev, data);
    // Yield to the other processes, so that they have a chance to take broadcast events out of the event queue
    PROCESS_PAUSE();
  }

  PROCESS_END();
}
