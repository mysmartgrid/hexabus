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
#include "net/uip-udp-packet.h"

#include "state_machine.h"
#include "datetime_service.h"
#include "endpoint_registry.h"
#include "hexabus_config.h"
#include "sequence_numbers.h"
#include "reliability.h"

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

static size_t prepare_for_send(union hxb_packet_any* packet, const uip_ipaddr_t* toaddr, uint16_t toport)
{
	size_t len;

	strncpy(packet->header.magic, HXB_HEADER, strlen(HXB_HEADER));
	if((packet->header.sequence_number) == 0) {
		packet->header.sequence_number = next_sequence_number(toaddr, toport);
	}

	syslog(LOG_DEBUG, "Sending sequence number %u", packet->header.sequence_number);

	packet->header.sequence_number = uip_htons(packet->header.sequence_number);

	switch ((enum hxb_packet_type) packet->header.type) {
	case HXB_PTYPE_INFO:
	case HXB_PTYPE_WRITE:
		packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
		switch ((enum hxb_datatype) packet->value_header.datatype) {
		#define HANDLE_PACKET(DTYPE, PTYPE, CONV, BITEXTRACT) \
			case DTYPE: \
				len = sizeof(packet->PTYPE); \
				packet->PTYPE.value = CONV(BITEXTRACT(packet->PTYPE.value)); \
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
			break;

		case HXB_DTYPE_65BYTES:
			len = sizeof(packet->p_65bytes);
			break;

		case HXB_DTYPE_16BYTES:
			len = sizeof(packet->p_16bytes);
			break;

		case HXB_DTYPE_UNDEFINED:
			return 0;
		}
		break;

	case HXB_PTYPE_REPORT:
		packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
		switch ((enum hxb_datatype) packet->value_header.datatype) {
		#define HANDLE_PACKET(DTYPE, PTYPE, CONV, BITEXTRACT) \
			case DTYPE: \
				len = sizeof(packet->PTYPE); \
				packet->PTYPE.value = CONV(BITEXTRACT(packet->PTYPE.value)); \
				packet->PTYPE.cause_sequence_number = uip_htons(packet->PTYPE.cause_sequence_number); \
				break;

		HANDLE_PACKET(HXB_DTYPE_BOOL, p_report_u8, , )
		HANDLE_PACKET(HXB_DTYPE_UINT8, p_report_u8, , )
		HANDLE_PACKET(HXB_DTYPE_UINT16, p_report_u16, uip_htons, )
		HANDLE_PACKET(HXB_DTYPE_UINT32, p_report_u32, uip_htonl, )
		HANDLE_PACKET(HXB_DTYPE_UINT64, p_report_u64, htonul, )
		HANDLE_PACKET(HXB_DTYPE_SINT8, p_report_s8, , bits_from_signed8)
		HANDLE_PACKET(HXB_DTYPE_SINT16, p_report_s16, uip_htons, bits_from_signed16)
		HANDLE_PACKET(HXB_DTYPE_SINT32, p_report_s32, uip_htonl, bits_from_signed32)
		HANDLE_PACKET(HXB_DTYPE_SINT64, p_report_s64, htonul, bits_from_signed64)
		HANDLE_PACKET(HXB_DTYPE_FLOAT, p_report_float, htonf, )

		#undef HANDLE_PACKET

		case HXB_DTYPE_128STRING:
			len = sizeof(packet->p_report_128string);
			packet->p_report_128string.cause_sequence_number = uip_htons(packet->p_report_128string.cause_sequence_number);
			break;

		case HXB_DTYPE_65BYTES:
			len = sizeof(packet->p_report_65bytes);
			packet->p_report_65bytes.cause_sequence_number = uip_htons(packet->p_report_65bytes.cause_sequence_number);
			break;

		case HXB_DTYPE_16BYTES:
			len = sizeof(packet->p_report_16bytes);
			packet->p_report_16bytes.cause_sequence_number = uip_htons(packet->p_report_16bytes.cause_sequence_number);
			break;

		case HXB_DTYPE_UNDEFINED:
			return 0;
		}
		break;

	case HXB_PTYPE_EP_PROP_REPORT:
		packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
		switch ((enum hxb_datatype) packet->value_header.datatype) {
		#define HANDLE_PACKET(DTYPE, PTYPE, CONV, BITEXTRACT) \
			case DTYPE: \
				len = sizeof(packet->PTYPE); \
				packet->PTYPE.value = CONV(BITEXTRACT(packet->PTYPE.value)); \
				packet->PTYPE.next_id = uip_htonl(packet->PTYPE.next_id); \
				packet->PTYPE.cause_sequence_number = uip_htons(packet->PTYPE.cause_sequence_number); \
				break;

		HANDLE_PACKET(HXB_DTYPE_BOOL, p_preport_u8, , )
		HANDLE_PACKET(HXB_DTYPE_UINT8, p_preport_u8, , )
		HANDLE_PACKET(HXB_DTYPE_UINT16, p_preport_u16, uip_htons, )
		HANDLE_PACKET(HXB_DTYPE_UINT32, p_preport_u32, uip_htonl, )
		HANDLE_PACKET(HXB_DTYPE_UINT64, p_preport_u64, htonul, )
		HANDLE_PACKET(HXB_DTYPE_SINT8, p_preport_s8, , bits_from_signed8)
		HANDLE_PACKET(HXB_DTYPE_SINT16, p_preport_s16, uip_htons, bits_from_signed16)
		HANDLE_PACKET(HXB_DTYPE_SINT32, p_preport_s32, uip_htonl, bits_from_signed32)
		HANDLE_PACKET(HXB_DTYPE_SINT64, p_preport_s64, htonul, bits_from_signed64)
		HANDLE_PACKET(HXB_DTYPE_FLOAT, p_preport_float, htonf, )

		#undef HANDLE_PACKET

		case HXB_DTYPE_128STRING:
			len = sizeof(packet->p_preport_128string);
			packet->p_preport_128string.next_id = uip_htonl(packet->p_preport_128string.next_id);
			packet->p_preport_128string.cause_sequence_number = uip_htons(packet->p_preport_128string.cause_sequence_number);
			break;

		case HXB_DTYPE_65BYTES:
			len = sizeof(packet->p_preport_65bytes);
			packet->p_preport_65bytes.next_id = uip_htonl(packet->p_preport_65bytes.next_id);
			packet->p_preport_65bytes.cause_sequence_number = uip_htons(packet->p_preport_65bytes.cause_sequence_number);
			break;

		case HXB_DTYPE_16BYTES:
			len = sizeof(packet->p_preport_16bytes);
			packet->p_preport_16bytes.next_id = uip_htonl(packet->p_preport_16bytes.next_id);
			packet->p_preport_16bytes.cause_sequence_number = uip_htons(packet->p_preport_16bytes.cause_sequence_number);
			break;

		case HXB_DTYPE_UNDEFINED:
			return 0;
		}
		break;

	case HXB_PTYPE_EPQUERY:
	case HXB_PTYPE_QUERY:
		len = sizeof(packet->p_query);
		packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
		break;

	case HXB_PTYPE_EPINFO:
		len = sizeof(packet->p_128string);
		packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
		break;

	case HXB_PTYPE_EPREPORT:
		len = sizeof(packet->p_report_128string);
		packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
		packet->p_report_128string.cause_sequence_number = uip_htons(packet->p_report_128string.cause_sequence_number);
		break;

	case HXB_PTYPE_ERROR:
		len = sizeof(packet->p_error);
		packet->p_error.cause_sequence_number = uip_htons(packet->p_error.cause_sequence_number);
		break;

	case HXB_PTYPE_ACK:
		len = sizeof(packet->p_ack);
		packet->p_ack.cause_sequence_number = uip_htons(packet->p_ack.cause_sequence_number);
		break;

	case HXB_PTYPE_PINFO:
	case HXB_PTYPE_EP_PROP_WRITE:
	case HXB_PTYPE_EP_PROP_QUERY: //TODO this one should?
		//should not be sent from device
		return 0;
	}

	return len;
}

void do_udp_send(const uip_ipaddr_t* toaddr, uint16_t toport, union hxb_packet_any* packet)
{
	size_t len = prepare_for_send(packet, toaddr, toport);

	if (len == 0) {
		syslog(LOG_ERR, "Attempted to send invalid packet");
		return;
	}

	if (!udpconn) {
		syslog(LOG_ERR, "Not sending: UDP connection not available");
		return;
	}


	uip_udp_packet_sendto(udpconn, packet, len, toaddr, UIP_HTONS(toport));
}

enum hxb_error_code udp_handler_send_generated(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code (*packet_gen_fn)(union hxb_packet_any* buffer, void* data), void* data)
{
	enum hxb_error_code err;
	union hxb_packet_any send_buffer;

	if (!toaddr) {
		toaddr = &hxb_group;
	}

	if (!toport) {
		toport = HXB_PORT;
	}

	send_buffer.header.sequence_number = 0;
	send_buffer.header.flags = HXB_FLAG_NONE;

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

enum hxb_error_code udp_handler_send_generated_reliable(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code (*packet_gen_fn)(union hxb_packet_any* buffer, void* data), void* data)

{
	enum hxb_error_code err;
	union hxb_packet_any send_buffer;

	if (!toaddr) {
		toaddr = &hxb_group;
	}

	if (!toport) {
		toport = HXB_PORT;
	}

	send_buffer.header.sequence_number = 0;
	send_buffer.header.flags = HXB_FLAG_WANT_ACK;

	if (!packet_gen_fn) {
		syslog(LOG_ERR, "Attempted to generate packet by NULL packet_gen_fn");
		return HXB_ERR_INTERNAL;
	}

	err = packet_gen_fn(&send_buffer, data);
	if (err) {
		return err;
	}

	enqueue_packet(toaddr, toport, &send_buffer);

	return HXB_ERR_SUCCESS;
}

enum hxb_error_code udp_handler_send_error(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code code, uint16_t cause_sequence_number)
{
	struct hxb_packet_error err = {
		.type = HXB_PTYPE_ERROR,
		.cause_sequence_number = cause_sequence_number,
		.error_code = code
	};

	do_udp_send(toaddr, toport, (union hxb_packet_any*) &err);

	return HXB_ERR_SUCCESS;
}

enum hxb_error_code udp_handler_send_ack(const uip_ipaddr_t* toaddr, uint16_t toport, uint16_t cause_sequence_number)
{
	syslog(LOG_INFO, "Sent explicit ACK %u", cause_sequence_number);
	struct hxb_packet_ack ack = {
		.sequence_number = 0,
		.type = HXB_PTYPE_ACK,
		.cause_sequence_number = cause_sequence_number
	};

	do_udp_send(toaddr, toport, (union hxb_packet_any*) &ack);

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
	uint32_t eid = ((struct eid_cause*) data)->eid;
	uint16_t cause_sequence_number = ((struct eid_cause*) data)->cause_sequence_number;

	struct hxb_value value;
	enum hxb_error_code err;

	buffer->value_header.type = HXB_PTYPE_REPORT;
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
			buffer->PTYPE.cause_sequence_number = cause_sequence_number; \
			break;

	HANDLE_PACKET(HXB_DTYPE_BOOL, p_report_u8, v_u8)
	HANDLE_PACKET(HXB_DTYPE_UINT8, p_report_u8, v_u8)
	HANDLE_PACKET(HXB_DTYPE_UINT16, p_report_u16, v_u16)
	HANDLE_PACKET(HXB_DTYPE_UINT32, p_report_u32, v_u32)
	HANDLE_PACKET(HXB_DTYPE_UINT64, p_report_u64, v_u64)
	HANDLE_PACKET(HXB_DTYPE_SINT8, p_report_s8, v_s8)
	HANDLE_PACKET(HXB_DTYPE_SINT16, p_report_s16, v_s16)
	HANDLE_PACKET(HXB_DTYPE_SINT32, p_report_s32, v_s32)
	HANDLE_PACKET(HXB_DTYPE_SINT64, p_report_s64, v_s64)
	HANDLE_PACKET(HXB_DTYPE_FLOAT, p_report_float, v_float)

	#undef HANDLE_PACKET

	// these just work, because we pointed v_string and v_binary at the appropriate field in the
	// packet union.
	case HXB_DTYPE_128STRING:
		buffer->p_128string.value[HXB_STRING_PACKET_BUFFER_LENGTH] = 0;
		buffer->p_report_128string.cause_sequence_number = cause_sequence_number;
		break;

	case HXB_DTYPE_65BYTES:
		buffer->p_report_65bytes.cause_sequence_number = cause_sequence_number;
		break;
	case HXB_DTYPE_16BYTES:
		buffer->p_report_16bytes.cause_sequence_number = cause_sequence_number;
		break;

	case HXB_DTYPE_UNDEFINED:
		return HXB_ERR_DATATYPE;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code generate_epquery_response(union hxb_packet_any* buffer, void* data)
{
	buffer->p_report_128string.type = HXB_PTYPE_EPREPORT;
	buffer->p_report_128string.eid = ((struct eid_cause*) data)->eid;
	buffer->p_report_128string.cause_sequence_number = ((struct eid_cause*) data)->cause_sequence_number;
	buffer->p_report_128string.datatype = endpoint_get_datatype(buffer->p_report_128string.eid);
	if (buffer->p_report_128string.datatype == HXB_DTYPE_UNDEFINED) {
		return HXB_ERR_UNKNOWNEID;
	}

	enum hxb_error_code err;

	err = endpoint_get_name(buffer->p_report_128string.eid, buffer->p_report_128string.value, HXB_STRING_PACKET_BUFFER_LENGTH);
	buffer->p_report_128string.value[HXB_STRING_PACKET_BUFFER_LENGTH] = '\0';
	if (err) {
		return err;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code generate_propquery_response(union hxb_packet_any* buffer, void* data)
{
	uint32_t eid = ((struct prop_cause*) data)->eid;
	uint32_t propid = ((struct prop_cause*) data)->propid;
	uint16_t cause_sequence_number = ((struct prop_cause*) data)->cause_sequence_number;

	struct hxb_value value;
	enum hxb_error_code err;

	buffer->propreport_header.type = HXB_PTYPE_EP_PROP_REPORT;
	buffer->propreport_header.eid = eid;
	buffer->propreport_header.next_id = get_next_property(eid, propid);

	syslog(LOG_INFO, "Received prop query for eid %lu propid %lu", eid, propid);

	// point v_binary and v_string to the appropriate buffer in the source packet.
	// that way we avoid allocating another buffer and unneeded copies
	value.v_string = buffer->p_preport_128string.value;

	err = endpoint_property_read(eid, propid, &value);
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
			buffer->PTYPE.cause_sequence_number = cause_sequence_number; \
			break;

	HANDLE_PACKET(HXB_DTYPE_BOOL, p_preport_u8, v_u8)
	HANDLE_PACKET(HXB_DTYPE_UINT8, p_preport_u8, v_u8)
	HANDLE_PACKET(HXB_DTYPE_UINT16, p_preport_u16, v_u16)
	HANDLE_PACKET(HXB_DTYPE_UINT32, p_preport_u32, v_u32)
	HANDLE_PACKET(HXB_DTYPE_UINT64, p_preport_u64, v_u64)
	HANDLE_PACKET(HXB_DTYPE_SINT8, p_preport_s8, v_s8)
	HANDLE_PACKET(HXB_DTYPE_SINT16, p_preport_s16, v_s16)
	HANDLE_PACKET(HXB_DTYPE_SINT32, p_preport_s32, v_s32)
	HANDLE_PACKET(HXB_DTYPE_SINT64, p_preport_s64, v_s64)
	HANDLE_PACKET(HXB_DTYPE_FLOAT, p_preport_float, v_float)

	#undef HANDLE_PACKET

	// these just work, because we pointed v_string and v_binary at the appropriate field in the
	// packet union.
	case HXB_DTYPE_128STRING:
		buffer->p_preport_128string.value[HXB_STRING_PACKET_BUFFER_LENGTH] = 0;
		buffer->p_preport_128string.cause_sequence_number = cause_sequence_number;
		break;

	case HXB_DTYPE_65BYTES:
		buffer->p_preport_65bytes.cause_sequence_number = cause_sequence_number;
		break;
	case HXB_DTYPE_16BYTES:
		buffer->p_preport_16bytes.cause_sequence_number = cause_sequence_number;
		break;

	case HXB_DTYPE_UNDEFINED:
		return HXB_ERR_DATATYPE;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code handle_prop_write(union hxb_packet_any* packet)
{
	struct hxb_value value;
	value.datatype = packet->property_header.datatype;

	switch ((enum hxb_datatype) value.datatype) {
	#define HANDLE_PACKET(DTYPE, TARGET, PTYPE, CONV, BITEXTRACT) \
		case DTYPE: \
			value.TARGET = CONV(BITEXTRACT(packet->PTYPE.value)); \
			break;

	HANDLE_PACKET(HXB_DTYPE_BOOL, v_u8, p_pwrite_u8, , )
	HANDLE_PACKET(HXB_DTYPE_UINT8, v_u8, p_pwrite_u8, , )
	HANDLE_PACKET(HXB_DTYPE_UINT16, v_u16, p_pwrite_u16, uip_ntohs, )
	HANDLE_PACKET(HXB_DTYPE_UINT32, v_u32, p_pwrite_u32, uip_ntohl, )
	HANDLE_PACKET(HXB_DTYPE_UINT64, v_u64, p_pwrite_u64, ntohul, )
	HANDLE_PACKET(HXB_DTYPE_SINT8, v_s8, p_pwrite_s8, , bits_to_signed8)
	HANDLE_PACKET(HXB_DTYPE_SINT16, v_s16, p_pwrite_s16, uip_ntohs, bits_to_signed16)
	HANDLE_PACKET(HXB_DTYPE_SINT32, v_s32, p_pwrite_s32, uip_ntohl, bits_to_signed32)
	HANDLE_PACKET(HXB_DTYPE_SINT64, v_s64, p_pwrite_s64, ntohul, bits_to_signed64)
	HANDLE_PACKET(HXB_DTYPE_FLOAT, v_float, p_pwrite_float, ntohf, )
	HANDLE_PACKET(HXB_DTYPE_128STRING, v_string, p_pwrite_128string, , )
	HANDLE_PACKET(HXB_DTYPE_65BYTES, v_binary, p_pwrite_65bytes, , )
	HANDLE_PACKET(HXB_DTYPE_16BYTES, v_binary, p_pwrite_16bytes, , )

	#undef HANDLE_PACKET

	case HXB_DTYPE_UNDEFINED:
		return HXB_ERR_DATATYPE;
	}

	return endpoint_property_write(uip_ntohl(packet->property_header.eid),
		uip_ntohl(packet->property_header.property_id), &value);
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

static enum hxb_error_code handle_pinfo(union hxb_packet_any* packet)
{
	struct hxb_envelope envelope;
	enum hxb_error_code err;

	err = extract_value(packet, &envelope.value);
	if (err) {
		return HXB_ERR_SUCCESS;
	}

	switch ((enum hxb_datatype) envelope.value.datatype) {
	#define EXTRACT_ORIGIN(DTYPE, PTYPE) \
		case DTYPE: \
			memcpy(&envelope.src_ip.u8, &packet->PTYPE.origin, 16); \
			break;

	EXTRACT_ORIGIN(HXB_DTYPE_BOOL, p_proxy_u8)
	EXTRACT_ORIGIN(HXB_DTYPE_UINT8, p_proxy_u8)
	EXTRACT_ORIGIN(HXB_DTYPE_UINT16, p_proxy_u16)
	EXTRACT_ORIGIN(HXB_DTYPE_UINT32, p_proxy_u32)
	EXTRACT_ORIGIN(HXB_DTYPE_UINT64, p_proxy_u64)
	EXTRACT_ORIGIN(HXB_DTYPE_SINT8, p_proxy_s8)
	EXTRACT_ORIGIN(HXB_DTYPE_SINT16, p_proxy_s16)
	EXTRACT_ORIGIN(HXB_DTYPE_SINT32, p_proxy_s32)
	EXTRACT_ORIGIN(HXB_DTYPE_SINT64, p_proxy_s64)
	EXTRACT_ORIGIN(HXB_DTYPE_FLOAT, p_proxy_float)
	EXTRACT_ORIGIN(HXB_DTYPE_128STRING, p_proxy_128string)
	EXTRACT_ORIGIN(HXB_DTYPE_65BYTES, p_proxy_65bytes)
	EXTRACT_ORIGIN(HXB_DTYPE_16BYTES, p_proxy_16bytes)
	#undef EXTRACT_ORIGIN

	case HXB_DTYPE_UNDEFINED:
		return HXB_ERR_DATATYPE;
	}

	envelope.src_port = uip_ntohs(UDP_IP_BUF->srcport);
	envelope.eid = uip_ntohl(packet->eid_header.eid);


#if STATE_MACHINE_ENABLE
	sm_handle_input(&envelope);
#else
	syslog(LOG_DEBUG, "Received Broadcast, but no handler for datatype.");
#endif

	return HXB_ERR_SUCCESS;
}

void udp_handler_handle_incoming(struct hxb_queue_packet* R) {

	enum hxb_error_code err;
	union hxb_packet_any* packet = &(R->packet);

	// don't send error packets for broadcasts
	bool is_broadcast = uip_ipaddr_cmp(&UDP_IP_BUF->destipaddr, &hxb_group);

	syslog(LOG_DEBUG, "Handling packet of type %x", packet->header.type);

	switch ((enum hxb_packet_type) packet->header.type) {
		case HXB_PTYPE_WRITE:
			err = handle_write(packet);
			if(!err && (packet->header.flags & HXB_FLAG_WANT_ACK))
				udp_handler_send_ack(&(R->ip), R->port, uip_ntohs(packet->header.sequence_number));
			break;

		case HXB_PTYPE_QUERY:
			{
				struct eid_cause ec = {
					.eid = uip_ntohl(packet->p_query.eid),
					.cause_sequence_number = uip_ntohs(packet->p_query.sequence_number),
				};
				err = udp_handler_send_generated(&(R->ip), R->port, generate_query_response, &ec);
				break;
			}

		case HXB_PTYPE_EPQUERY:
			{
				struct eid_cause ec = {
					.eid = uip_ntohl(packet->p_query.eid),
					.cause_sequence_number = uip_ntohs(packet->p_query.sequence_number),
				};
				err = udp_handler_send_generated(&(R->ip), R->port, generate_epquery_response, &ec);
				break;
			}

		case HXB_PTYPE_INFO:
		case HXB_PTYPE_REPORT:
		case HXB_PTYPE_EPREPORT:
			err = handle_info(packet);
			break;
		case HXB_PTYPE_PINFO:
			if(uip_ipaddr_cmp(&(R->ip), &udp_master_addr)) {
				ul_send_ack(&(R->ip), R->port, uip_ntohs(packet->header.sequence_number));
				err = handle_pinfo(packet);
			} else {
				err = HXB_ERR_UNEXPECTED_PACKET;
			}
			break;
		case HXB_PTYPE_ACK:
			ul_ack_received(&hxb_group, HXB_PORT, uip_ntohs(packet->p_ack.cause_sequence_number));
			err = HXB_ERR_SUCCESS;
			break;

		case HXB_PTYPE_EP_PROP_QUERY:
			{
				struct prop_cause pc = {
					.eid = uip_ntohl(packet->p_pquery.eid),
					.propid = uip_ntohl(packet->p_pquery.property_id),
					.cause_sequence_number = uip_ntohs(packet->p_query.sequence_number),
				};
				err = udp_handler_send_generated(&(R->ip), R->port, generate_propquery_response, &pc);
				break;
			}

		case HXB_PTYPE_EP_PROP_WRITE:
			{
				err = handle_prop_write(packet);
				if(!err && (packet->header.flags & HXB_FLAG_WANT_ACK))
					udp_handler_send_ack(&(R->ip), R->port, uip_ntohs(packet->header.sequence_number));
				break;
			}

		case HXB_PTYPE_ERROR:
		case HXB_PTYPE_EPINFO:
		case HXB_PTYPE_EP_PROP_REPORT:
		default:
			syslog(LOG_NOTICE, "packet of type %d received, but we do not know what to do with that (yet)", packet->header.type);
			err = HXB_ERR_UNEXPECTED_PACKET;
			break;
	}

	if (err) {
		if (!is_broadcast && !(err & HXB_ERR_INTERNAL)) {
			udp_handler_send_error(&(R->ip), R->port, err, uip_ntohs(packet->header.sequence_number));
		}
		return;
	}
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
			syslog(LOG_DEBUG, "LQI: %u RSSI: %u", packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY), packetbuf_attr(PACKETBUF_ATTR_RSSI));

			union hxb_packet_any* packet = (union hxb_packet_any*)uip_appdata;

      // check if it's a Hexabus packet
      if(strncmp(packet->header.magic, HXB_HEADER, 4)) {
				syslog(LOG_NOTICE, "Received something, but it wasn't a Hexabus packet. Ignoring it.");
      } else {
				struct hxb_queue_packet R = {
					.ip = UDP_IP_BUF->srcipaddr,
					.port = uip_ntohs(UDP_IP_BUF->srcport),
					.packet = *packet,
				};

				receive_packet(&R);
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

	sequence_number_init();

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

  //find master address
  uip_ip6addr(&udp_master_addr, 0xfe80,0,0,0,0,0,0,1);
	for (int i = 0; i < UIP_DS6_ADDR_NB; i++) {
		if (uip_ds6_if.addr_list[i].isused && uip_ds6_if.addr_list[i].ipaddr.u16[0]!=0xfe80) {
			udp_master_addr.u16[0] = uip_ds6_if.addr_list[i].ipaddr.u16[0];
			udp_master_addr.u16[1] = uip_ds6_if.addr_list[i].ipaddr.u16[1];
			udp_master_addr.u16[2] = uip_ds6_if.addr_list[i].ipaddr.u16[2];
			udp_master_addr.u16[3] = uip_ds6_if.addr_list[i].ipaddr.u16[3];
			break;
		}
	}
	syslog(LOG_INFO, "Setting master address to " LOG_6ADDR_FMT, LOG_6ADDR_VAL(udp_master_addr));

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
