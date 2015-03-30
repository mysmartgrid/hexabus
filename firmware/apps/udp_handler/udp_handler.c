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
#include "lib/crc16.h"
#include "net/uip-udp-packet.h"

#include "state_machine.h"
#include "datetime_service.h"
#include "endpoint_registry.h"
#include "hexabus_config.h"

#define LOG_LEVEL UDP_HANDLER_DEBUG
#include "syslog.h"

#include "health.h"

#define UDP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

static struct uip_udp_conn *udpconn;
static struct etimer udp_unreachable_timer;
static udp_handler_event_t state;

static void reset_unreachable_timer()
{
	etimer_set(&udp_unreachable_timer, 2 * CLOCK_CONF_SECOND);
}

static uip_ipaddr_t hxb_group;

PROCESS(udp_handler_process, "HEXABUS Socket UDP handler Process");
AUTOSTART_PROCESSES(&udp_handler_process);

/*---------------------------------------------------------------------------*/
static void
pollhandler(void) {
	syslog(LOG_DEBUG, "Process polled");
}

static void
exithandler(void) {
	syslog(LOG_DEBUG, "Process exits.");
}

static float ntohf(float f)
{
	union {
		float    f;
		uint32_t u;
	} fconv = { f };
	fconv.u = uip_ntohl(fconv.u);
	return fconv.f;
}

static float htonf(float f)
{
	union {
		float    f;
		uint32_t u;
	} fconv = { f };
	fconv.u = uip_htonl(fconv.u);
	return fconv.f;
}

static uint64_t ntohul(uint64_t u)
{
	return (((uint64_t) uip_ntohl(u >> 32))) | (((uint64_t) uip_ntohl(u & 0xFFFFFFFF)) << 32);
}

static uint64_t htonul(uint64_t u)
{
	return (((uint64_t) uip_htonl(u >> 32))) | (((uint64_t) uip_htonl(u & 0xFFFFFFFF)) << 32);
}

#define BITS_CONV(bitness) \
	static uint##bitness##_t bits_from_signed##bitness(int##bitness##_t val) \
	{ \
		uint##bitness##_t result; \
		memcpy(&result, &val, sizeof(result)); \
		return result; \
	} \
	static int##bitness##_t bits_to_signed##bitness(uint##bitness##_t val) \
	{ \
		int##bitness##_t result; \
		memcpy(&result, &val, sizeof(result)); \
		return result; \
	}

BITS_CONV(8)
BITS_CONV(16)
BITS_CONV(32)
BITS_CONV(64)

#undef BITS_CONV

static size_t prepare_for_send(union hxb_packet_any* packet)
{
	size_t len;

	strncpy(packet->header.magic, HXB_HEADER, strlen(HXB_HEADER));
	packet->header.flags = 0;

	switch ((enum hxb_packet_type) packet->header.type) {
	case HXB_PTYPE_INFO:
	case HXB_PTYPE_WRITE:
		packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
		switch ((enum hxb_datatype) packet->value_header.datatype) {
		#define HANDLE_PACKET(DTYPE, PTYPE, CONV, BITEXTRACT) \
			case DTYPE: \
				len = sizeof(packet->PTYPE); \
				packet->PTYPE.value = CONV(BITEXTRACT(packet->PTYPE.value)); \
				packet->PTYPE.crc = uip_htons(crc16_data((unsigned char*) packet, len - 2, 0)); \
				break;

		HANDLE_PACKET(HXB_DTYPE_BOOL, p_u8, , )
		HANDLE_PACKET(HXB_DTYPE_UINT8, p_u8, , )
		HANDLE_PACKET(HXB_DTYPE_UINT16, p_u16, uip_htons, )
		HANDLE_PACKET(HXB_DTYPE_UINT32, p_u32, uip_htonl, )
		HANDLE_PACKET(HXB_DTYPE_UINT64, p_u64, htonul, )
		HANDLE_PACKET(HXB_DTYPE_SINT8, p_s8, , bits_from_signed8)
		HANDLE_PACKET(HXB_DTYPE_SINT16, p_s16, uip_htons, bits_from_signed16)
		HANDLE_PACKET(HXB_DTYPE_SINT32, p_s32, uip_htonl, bits_from_signed32)
		HANDLE_PACKET(HXB_DTYPE_SINT64, p_s64, htonul, bits_from_signed64)
		HANDLE_PACKET(HXB_DTYPE_FLOAT, p_float, htonf, )

		#undef HANDLE_PACKET

		case HXB_DTYPE_128STRING:
			len = sizeof(packet->p_128string);
			packet->p_128string.crc = uip_htons(crc16_data((unsigned char*) packet, len - 2, 0));
			break;

		case HXB_DTYPE_65BYTES:
			len = sizeof(packet->p_65bytes);
			packet->p_65bytes.crc = uip_htons(crc16_data((unsigned char*) packet, len - 2, 0));
			break;

		case HXB_DTYPE_16BYTES:
			len = sizeof(packet->p_16bytes);
			packet->p_16bytes.crc = uip_htons(crc16_data((unsigned char*) packet, len - 2, 0));
			break;

		case HXB_DTYPE_UNDEFINED:
			return 0;
		}
		break;

	case HXB_PTYPE_EPQUERY:
	case HXB_PTYPE_QUERY:
		len = sizeof(packet->p_query);
		packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
		packet->p_query.crc = uip_htons(crc16_data((unsigned char*) packet, len - 2, 0));
		break;

	case HXB_PTYPE_EPINFO:
		len = sizeof(packet->p_128string);
		packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
		packet->p_128string.crc = uip_htons(crc16_data((unsigned char*) packet, len - 2, 0));
		break;

	case HXB_PTYPE_ERROR:
		len = sizeof(packet->p_error);
		packet->p_error.crc = uip_htons(crc16_data((unsigned char*) packet, len - 2, 0));
		break;
	}

	return len;
}

static void do_udp_send(const uip_ipaddr_t* toaddr, uint16_t toport, union hxb_packet_any* packet)
{
	size_t len = prepare_for_send(packet);

	if (len == 0) {
		syslog(LOG_ERR, "Attempted to send invalid packet");
		return;
	}

	if (!udpconn) {
		syslog(LOG_ERR, "Not sending: UDP connection not available");
		return;
	}

	if (!toaddr) {
		toaddr = &hxb_group;
	}

	if (!toport) {
		toport = HXB_PORT;
	}

	uip_udp_packet_sendto(udpconn, packet, len, toaddr, UIP_HTONS(toport));
}

enum hxb_error_code udp_handler_send_generated(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code (*packet_gen_fn)(union hxb_packet_any* buffer, void* data), void* data)
{
	enum hxb_error_code err;
	union hxb_packet_any send_buffer;

	if (!packet_gen_fn) {
		syslog(LOG_ERR, "Attempted to generate packet by NULL packet_gen_fn");
		return HXB_ERR_INTERNAL;
	}

	err = packet_gen_fn(&send_buffer, data);
	if (err) {
		return err;
	}

	do_udp_send(toaddr, toport, &send_buffer);

	return HXB_ERR_SUCCESS;
}

enum hxb_error_code udp_handler_send_error(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code code)
{
	struct hxb_packet_error err = {
		.type = HXB_PTYPE_ERROR,
		.error_code = code
	};

	do_udp_send(toaddr, toport, (union hxb_packet_any*) &err);

	return HXB_ERR_SUCCESS;
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
		#define HANDLE_PACKET(DTYPE, PTYPE) \
			case DTYPE: \
				data_len = sizeof(packet->PTYPE); \
				crc = packet->PTYPE.crc; \
				break;

		HANDLE_PACKET(HXB_DTYPE_BOOL, p_u8)
		HANDLE_PACKET(HXB_DTYPE_UINT8, p_u8)
		HANDLE_PACKET(HXB_DTYPE_UINT16, p_u16)
		HANDLE_PACKET(HXB_DTYPE_UINT32, p_u32)
		HANDLE_PACKET(HXB_DTYPE_UINT64, p_u64)
		HANDLE_PACKET(HXB_DTYPE_SINT8, p_s8)
		HANDLE_PACKET(HXB_DTYPE_SINT16, p_s16)
		HANDLE_PACKET(HXB_DTYPE_SINT32, p_s32)
		HANDLE_PACKET(HXB_DTYPE_SINT64, p_s64)
		HANDLE_PACKET(HXB_DTYPE_FLOAT, p_float)
		HANDLE_PACKET(HXB_DTYPE_128STRING, p_128string)
		HANDLE_PACKET(HXB_DTYPE_65BYTES, p_65bytes)
		HANDLE_PACKET(HXB_DTYPE_16BYTES, p_16bytes)

		#undef HANDLE_PACKET

		case HXB_DTYPE_UNDEFINED:
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
		syslog(LOG_NOTICE, "CRC check failed");
		return HXB_ERR_CRCFAILED;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code extract_value(union hxb_packet_any* packet, struct hxb_value* value)
{
	value->datatype = packet->value_header.datatype;

	switch ((enum hxb_datatype) value->datatype) {
	#define HANDLE_PACKET(DTYPE, TARGET, PTYPE, CONV, BITEXTRACT) \
		case DTYPE: \
			value->TARGET = CONV(BITEXTRACT(packet->PTYPE.value)); \
			break;

	HANDLE_PACKET(HXB_DTYPE_BOOL, v_u8, p_u8, , )
	HANDLE_PACKET(HXB_DTYPE_UINT8, v_u8, p_u8, , )
	HANDLE_PACKET(HXB_DTYPE_UINT16, v_u16, p_u16, uip_ntohs, )
	HANDLE_PACKET(HXB_DTYPE_UINT32, v_u32, p_u32, uip_ntohl, )
	HANDLE_PACKET(HXB_DTYPE_UINT64, v_u64, p_u64, ntohul, )
	HANDLE_PACKET(HXB_DTYPE_SINT8, v_s8, p_s8, , bits_to_signed8)
	HANDLE_PACKET(HXB_DTYPE_SINT16, v_s16, p_s16, uip_ntohs, bits_to_signed16)
	HANDLE_PACKET(HXB_DTYPE_SINT32, v_s32, p_s32, uip_ntohl, bits_to_signed32)
	HANDLE_PACKET(HXB_DTYPE_SINT64, v_s64, p_s64, ntohul, bits_to_signed64)
	HANDLE_PACKET(HXB_DTYPE_FLOAT, v_float, p_float, ntohf, )
	HANDLE_PACKET(HXB_DTYPE_128STRING, v_string, p_128string, , )
	HANDLE_PACKET(HXB_DTYPE_65BYTES, v_binary, p_65bytes, , )
	HANDLE_PACKET(HXB_DTYPE_16BYTES, v_binary, p_16bytes, , )

	#undef HANDLE_PACKET

	case HXB_DTYPE_UNDEFINED:
	default:
		syslog(LOG_ERR, "Packet of unknown datatype.");
		return HXB_ERR_DATATYPE;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code handle_write(union hxb_packet_any* packet)
{
	struct hxb_envelope env;

	uip_ipaddr_copy(&env.src_ip, &UDP_IP_BUF->srcipaddr);
	env.src_port = uip_ntohs(UDP_IP_BUF->srcport);
	env.eid = uip_ntohl(packet->value_header.eid);

	enum hxb_error_code err;
	
	err = extract_value(packet, &env.value);
	if (err) {
		return err;
	}

	return endpoint_write(env.eid, &env);
}

static enum hxb_error_code generate_query_response(union hxb_packet_any* buffer, void* data)
{
	uint32_t eid = *((uint32_t*) data);

	struct hxb_value value;
	enum hxb_error_code err;

	buffer->value_header.type = HXB_PTYPE_INFO;
	buffer->value_header.eid = eid;

	syslog(LOG_INFO, "Received query for %lu", eid);

	// point v_binary and v_string to the appropriate buffer in the source packet.
	// that way we avoid allocating another buffer and unneeded copies
	value.v_string = buffer->p_128string.value;

	err = endpoint_read(eid, &value);
	if (err) {
		return err;
	}

	if (uip_ipaddr_cmp(&UDP_IP_BUF->destipaddr, &hxb_group)) {
		syslog(LOG_INFO, "Group query!");
		clock_delay_usec(random_rand() >> 1); // wait for max 31ms
	}

	buffer->value_header.datatype = value.datatype;
	switch ((enum hxb_datatype) value.datatype) {
	#define HANDLE_PACKET(DTYPE, PTYPE, SRC) \
		case DTYPE: \
			buffer->PTYPE.value = value.SRC; \
			break;

	HANDLE_PACKET(HXB_DTYPE_BOOL, p_u8, v_u8)
	HANDLE_PACKET(HXB_DTYPE_UINT8, p_u8, v_u8)
	HANDLE_PACKET(HXB_DTYPE_UINT16, p_u16, v_u16)
	HANDLE_PACKET(HXB_DTYPE_UINT32, p_u32, v_u32)
	HANDLE_PACKET(HXB_DTYPE_UINT64, p_u64, v_u64)
	HANDLE_PACKET(HXB_DTYPE_SINT8, p_s8, v_s8)
	HANDLE_PACKET(HXB_DTYPE_SINT16, p_s16, v_s16)
	HANDLE_PACKET(HXB_DTYPE_SINT32, p_s32, v_s32)
	HANDLE_PACKET(HXB_DTYPE_SINT64, p_s64, v_s64)
	HANDLE_PACKET(HXB_DTYPE_FLOAT, p_float, v_float)

	#undef HANDLE_PACKET

	// these just work, because we pointed v_string and v_binary at the appropriate field in the
	// packet union.
	case HXB_DTYPE_128STRING:
		buffer->p_128string.value[HXB_STRING_PACKET_BUFFER_LENGTH] = 0;
		break;

	case HXB_DTYPE_65BYTES:
	case HXB_DTYPE_16BYTES:
		break;

	case HXB_DTYPE_UNDEFINED:
		return HXB_ERR_DATATYPE;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code generate_epquery_response(union hxb_packet_any* buffer, void* data)
{
	buffer->p_128string.type = HXB_PTYPE_EPINFO;
	buffer->p_128string.eid = *((uint32_t*) data);
	buffer->p_128string.datatype = endpoint_get_datatype(buffer->p_128string.eid);
	if (buffer->p_128string.datatype == HXB_DTYPE_UNDEFINED) {
		return HXB_ERR_UNKNOWNEID;
	}

	enum hxb_error_code err;

	err = endpoint_get_name(buffer->p_128string.eid, buffer->p_128string.value, HXB_STRING_PACKET_BUFFER_LENGTH);
	buffer->p_128string.value[HXB_STRING_PACKET_BUFFER_LENGTH] = '\0';
	if (err) {
		return err;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code handle_info(union hxb_packet_any* packet)
{
	struct hxb_envelope envelope;
	enum hxb_error_code err;

	uip_ipaddr_copy(&envelope.src_ip, &UDP_IP_BUF->srcipaddr);
	envelope.src_port = uip_ntohs(UDP_IP_BUF->srcport);
	envelope.eid = uip_ntohl(packet->eid_header.eid);

	err = extract_value(packet, &envelope.value);
	if (err) {
		return HXB_ERR_SUCCESS;
	}

#if STATE_MACHINE_ENABLE
	sm_handle_input(&envelope);
#else
	syslog(LOG_DEBUG, "Received Broadcast, but no handler for datatype.");
#endif

	return HXB_ERR_SUCCESS;
}

static void
udphandler(process_event_t ev, process_data_t data)
{
	if (ev == PROCESS_EVENT_TIMER) {
		if (etimer_expired(&udp_unreachable_timer)) {
			reset_unreachable_timer();
			uip_ds6_defrt_t* rtr = uip_ds6_defrt_lookup(uip_ds6_defrt_choose());
			bool expired = stimer_expired(&rtr->lifetime);
			bool route_valid = rtr && (rtr->isinfinite || !expired);
			if (!route_valid && state != UDP_HANDLER_DOWN) {
				state = UDP_HANDLER_DOWN;
				process_post(PROCESS_BROADCAST, udp_handler_event, &state);
				health_update(HE_NO_CONNECTION, true);
			} else if (route_valid && state == UDP_HANDLER_DOWN) {
				state = UDP_HANDLER_UP;
				process_post(PROCESS_BROADCAST, udp_handler_event, &state);
				health_update(HE_NO_CONNECTION, false);
			}
		}
	} else if (ev == tcpip_event) {
    if(uip_newdata()) {
			syslog(LOG_DEBUG, "received '%d' bytes from " LOG_6ADDR_FMT, uip_datalen(), LOG_6ADDR_VAL(UDP_IP_BUF->srcipaddr));

			union hxb_packet_any* packet = (union hxb_packet_any*)uip_appdata;

      // check if it's a Hexabus packet
      if(strncmp(packet->header.magic, HXB_HEADER, 4)) {
				syslog(LOG_NOTICE, "Received something, but it wasn't a Hexabus packet. Ignoring it.");
      } else {
				enum hxb_error_code err;

				// don't send error packets for broadcasts
				bool is_broadcast = packet->header.type == HXB_PTYPE_INFO
					&& uip_ipaddr_cmp(&UDP_IP_BUF->destipaddr, &hxb_group);

				uip_ipaddr_t srcip = UDP_IP_BUF->srcipaddr;
				uint16_t srcport = uip_ntohs(UDP_IP_BUF->srcport);

				err = check_crc(packet);
				if (err) {
					if (!is_broadcast && !(err & HXB_ERR_INTERNAL)) {
						udp_handler_send_error(&srcip, srcport, err);
					}
					return;
				}

				switch ((enum hxb_packet_type) packet->header.type) {
					case HXB_PTYPE_WRITE:
						err = handle_write(packet);
						break;

					case HXB_PTYPE_QUERY:
						{
							uint32_t eid = uip_ntohl(packet->p_query.eid);
							uip_ipaddr_t src = UDP_IP_BUF->srcipaddr;
							err = udp_handler_send_generated(&srcip, srcport, generate_query_response, &eid);
							break;
						}

					case HXB_PTYPE_EPQUERY:
						{
							uint32_t eid = uip_ntohl(packet->p_query.eid);
							uip_ipaddr_t src = UDP_IP_BUF->srcipaddr;
							err = udp_handler_send_generated(&srcip, srcport, generate_epquery_response, &eid);
							break;
						}

					case HXB_PTYPE_INFO:
						err = handle_info(packet);
						break;

					case HXB_PTYPE_ERROR:
					case HXB_PTYPE_EPINFO:
					default:
						syslog(LOG_NOTICE, "packet of type %d received, but we do not know what to do with that (yet)", packet->header.type);
						err = HXB_ERR_UNEXPECTED_PACKET;
						break;
				}

				if (err) {
					if (!is_broadcast && !(err & HXB_ERR_INTERNAL)) {
						udp_handler_send_error(&srcip, srcport, err);
					}
					return;
				}
      }
    }
  }
}

static void print_local_addresses(void) {
	syslog(LOG_INFO, "Addresses [%u max]",UIP_DS6_ADDR_NB);
	for (int i = 0; i < UIP_DS6_ADDR_NB; i++) {
		if (uip_ds6_if.addr_list[i].isused) {
			syslog(LOG_INFO, " " LOG_6ADDR_FMT, LOG_6ADDR_VAL(uip_ds6_if.addr_list[i].ipaddr));
		}
	}
}

/*---------------------------------------------------------------------------*/

process_event_t udp_handler_event;

PROCESS_THREAD(udp_handler_process, ev, data) {
//  static uip_ipaddr_t ipaddr;
  PROCESS_POLLHANDLER(pollhandler());
  PROCESS_EXITHANDLER(exithandler());

  // see: http://senstools.gforge.inria.fr/doku.php?id=contiki:examples
  PROCESS_BEGIN();

	syslog(LOG_INFO, "process startup.");
	udp_handler_event = process_alloc_event();

  // wait 3 second, in order to have the IP addresses well configured
  etimer_set(&udp_unreachable_timer, CLOCK_CONF_SECOND*3);
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

	syslog(LOG_INFO, "Created connection with remote peer " LOG_6ADDR_FMT ", local/remote port %u/%u", LOG_6ADDR_VAL(udpconn->ripaddr), uip_htons(udpconn->lport),uip_htons(udpconn->rport));

  print_local_addresses();

	reset_unreachable_timer();
	state = UDP_HANDLER_UP;
	process_post(PROCESS_BROADCAST, udp_handler_event, &state);

  while(1) {
    //   tcpip_poll_udp(udpconn);
    PROCESS_WAIT_EVENT();
    udphandler(ev, data);
    // Yield to the other processes, so that they have a chance to take broadcast events out of the event queue
    PROCESS_PAUSE();
  }

  PROCESS_END();
}
