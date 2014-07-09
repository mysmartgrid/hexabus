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
#include <util/delay.h>
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
uip_ipaddr_t udp_master_addr;

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

static size_t prepare_for_send(union hxb_packet_any* packet, const uip_ipaddr_t* toaddr)
{
	size_t len;

	strncpy(packet->header.magic, HXB_HEADER, strlen(HXB_HEADER));
	if((packet->header.sequence_number) == 0) {
		packet->header.sequence_number = uip_htons(next_sequence_number(toaddr));
	}

	switch ((enum hxb_packet_type) packet->header.type) {
		case HXB_PTYPE_INFO:
		case HXB_PTYPE_WRITE:
			packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
			switch ((enum hxb_datatype) packet->value_header.datatype) {
				case HXB_DTYPE_UINT32:
					len = sizeof(packet->p_u32);
					packet->p_u32.value = uip_htonl(packet->p_u32.value);
					break;

				case HXB_DTYPE_FLOAT:
					len = sizeof(packet->p_float);
					packet->p_float.value = htonf(packet->p_float.value);
					break;

				case HXB_DTYPE_BOOL:
				case HXB_DTYPE_UINT8:
					len = sizeof(packet->p_u8);
					break;

				case HXB_DTYPE_128STRING:
					len = sizeof(packet->p_128string);
					break;

				case HXB_DTYPE_66BYTES:
					len = sizeof(packet->p_66bytes);
					break;

				case HXB_DTYPE_16BYTES:
					len = sizeof(packet->p_16bytes);
					break;

				case HXB_DTYPE_DATETIME:
				case HXB_DTYPE_TIMESTAMP:
				case HXB_DTYPE_UNDEFINED:
				default:
					return 0;
			}
			break;

		case HXB_PTYPE_REPORT:
		case HXB_PTYPE_EP_PROP_REPORT:
			packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
			switch ((enum hxb_datatype) packet->value_header.datatype) {
				case HXB_DTYPE_UINT32:
					len = sizeof(packet->p_u32_re);
					packet->p_u32_re.value = uip_htonl(packet->p_u32_re.value);
					packet->p_u32_re.cause_sequence_number = uip_htons(packet->p_u32_re.cause_sequence_number);
					break;

				case HXB_DTYPE_FLOAT:
					len = sizeof(packet->p_float_re);
					packet->p_float_re.value = htonf(packet->p_float_re.value);
					packet->p_float_re.cause_sequence_number = uip_htons(packet->p_float_re.cause_sequence_number);
					break;

				case HXB_DTYPE_BOOL:
				case HXB_DTYPE_UINT8:
					len = sizeof(packet->p_u8_re);
					packet->p_u8_re.cause_sequence_number = uip_htons(packet->p_u8_re.cause_sequence_number);
					break;

				case HXB_DTYPE_128STRING:
					len = sizeof(packet->p_128string_re);
					packet->p_128string_re.cause_sequence_number = uip_htons(packet->p_128string_re.cause_sequence_number);
					break;

				case HXB_DTYPE_66BYTES:
					len = sizeof(packet->p_66bytes_re);
					packet->p_66bytes_re.cause_sequence_number = uip_htons(packet->p_66bytes_re.cause_sequence_number);
					break;

				case HXB_DTYPE_16BYTES:
					len = sizeof(packet->p_16bytes_re);
					packet->p_16bytes_re.cause_sequence_number = uip_htons(packet->p_16bytes_re.cause_sequence_number);
					break;

				case HXB_DTYPE_DATETIME:
				case HXB_DTYPE_TIMESTAMP:
				case HXB_DTYPE_UNDEFINED:
				default:
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
			len = sizeof(packet->p_128string_re);
			packet->eid_header.eid = uip_htonl(packet->eid_header.eid);
			packet->p_128string_re.cause_sequence_number = uip_htons(packet->p_128string_re.cause_sequence_number);
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
		case HXB_PTYPE_TIMEINFO:
		case HXB_PTYPE_EP_PROP_WRITE:
		case HXB_PTYPE_EP_PROP_QUERY:
		//should not be sent from device
		default:
			return 0;
	}

	return len;
}

void do_udp_send(const uip_ipaddr_t* toaddr, uint16_t toport, union hxb_packet_any* packet)
{
	size_t len = prepare_for_send(packet, toaddr);

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

	// how big the actual value is depend on what type of packet we have.
	switch ((enum hxb_datatype) value->datatype) {
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			value->v_u8 = packet->p_u8.value;
			break;

		case HXB_DTYPE_UINT32:
			value->v_u32 = uip_ntohl(packet->p_u32.value);
			break;

		case HXB_DTYPE_FLOAT:
			value->v_float = ntohf(packet->p_float.value);
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

		case HXB_DTYPE_DATETIME:
		case HXB_DTYPE_TIMESTAMP:
			//TODO should not be used anymore
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
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			buffer->p_u8_re.value = value.v_u8;
			buffer->p_u8_re.cause_sequence_number = cause_sequence_number;
			break;

		case HXB_DTYPE_UINT32:
			buffer->p_u32_re.value = value.v_u32;
			buffer->p_u32_re.cause_sequence_number = cause_sequence_number;
			break;

		case HXB_DTYPE_FLOAT:
			buffer->p_float_re.value = value.v_float;
			buffer->p_float_re.cause_sequence_number = cause_sequence_number;
			break;

		// these just work, because we pointed v_string and v_binary at the appropriate field in the
		// packet union.
		case HXB_DTYPE_128STRING:
			buffer->p_128string_re.value[HXB_STRING_PACKET_MAX_BUFFER_LENGTH] = 0;
			buffer->p_128string_re.cause_sequence_number = cause_sequence_number;
			break;

		case HXB_DTYPE_66BYTES:
			buffer->p_66bytes_re.cause_sequence_number = cause_sequence_number;
			break;
		case HXB_DTYPE_16BYTES:
			buffer->p_16bytes_re.cause_sequence_number = cause_sequence_number;
			break;

		case HXB_DTYPE_DATETIME:
		case HXB_DTYPE_TIMESTAMP:
			//TODO should not be used anymore

		case HXB_DTYPE_UNDEFINED:
		default:
			return HXB_ERR_DATATYPE;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code generate_epquery_response(union hxb_packet_any* buffer, void* data)
{
	buffer->p_128string_re.type = HXB_PTYPE_EPREPORT;
	buffer->p_128string_re.eid = ((struct eid_cause*) data)->eid;
	buffer->p_128string_re.cause_sequence_number = ((struct eid_cause*) data)->cause_sequence_number;
	buffer->p_128string_re.datatype = endpoint_get_datatype(buffer->p_128string_re.eid);
	if (buffer->p_128string_re.datatype == HXB_DTYPE_UNDEFINED) {
		return HXB_ERR_UNKNOWNEID;
	}

	enum hxb_error_code err;

	err = endpoint_get_name(buffer->p_128string_re.eid, buffer->p_128string_re.value, HXB_STRING_PACKET_MAX_BUFFER_LENGTH);
	buffer->p_128string_re.value[HXB_STRING_PACKET_MAX_BUFFER_LENGTH] = '\0';
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

	syslog(LOG_INFO, "Received prop query for %lu", eid);

	// point v_binary and v_string to the appropriate buffer in the source packet.
	// that way we avoid allocating another buffer and unneeded copies
	value.v_string = buffer->p_128string.value;

	err = endpoint_property_read(eid, propid, &value);
	if (err) {
		return err;
	}

	if (uip_ipaddr_cmp(&UDP_IP_BUF->destipaddr, &hxb_group)) {
		syslog(LOG_INFO, "Group query!");
		clock_delay_usec(random_rand() >> 1); // wait for max 31ms
	}

	buffer->propreport_header.datatype = value.datatype;
	switch ((enum hxb_datatype) value.datatype) {
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			buffer->p_u8_prop_re.value = value.v_u8;
			buffer->p_u8_prop_re.cause_sequence_number = cause_sequence_number;
			break;

		case HXB_DTYPE_UINT32:
			buffer->p_u32_prop_re.value = value.v_u32;
			buffer->p_u32_prop_re.cause_sequence_number = cause_sequence_number;
			break;

		case HXB_DTYPE_FLOAT:
			buffer->p_float_prop_re.value = value.v_float;
			buffer->p_float_prop_re.cause_sequence_number = cause_sequence_number;
			break;

		// these just work, because we pointed v_string and v_binary at the appropriate field in the
		// packet union.
		case HXB_DTYPE_128STRING:
			buffer->p_128string_prop_re.value[HXB_STRING_PACKET_MAX_BUFFER_LENGTH] = 0;
			buffer->p_128string_prop_re.cause_sequence_number = cause_sequence_number;
			break;

		case HXB_DTYPE_66BYTES:
			buffer->p_66bytes_prop_re.cause_sequence_number = cause_sequence_number;
			break;
		case HXB_DTYPE_16BYTES:
			buffer->p_16bytes_prop_re.cause_sequence_number = cause_sequence_number;
			break;

		case HXB_DTYPE_DATETIME:
			buffer->p_datetime_prop_re.value = value.v_datetime;
			buffer->p_datetime_prop_re.cause_sequence_number = cause_sequence_number;
		case HXB_DTYPE_TIMESTAMP:
			buffer->p_u32_prop_re.value = value.v_timestamp;
			buffer->p_u32_prop_re.cause_sequence_number = cause_sequence_number;

		case HXB_DTYPE_UNDEFINED:
		default:
			return HXB_ERR_DATATYPE;
	}

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code handle_prop_write(union hxb_packet_any* packet)
{
	struct hxb_value value;

	value.datatype = packet->property_header.datatype;

	switch((enum hxb_datatype) value.datatype) {
		case HXB_DTYPE_BOOL:
		case HXB_DTYPE_UINT8:
			value.v_u8 = packet->p_u8_prop.value;
			break;

		case HXB_DTYPE_UINT32:
			value.v_u32 = uip_ntohl(packet->p_u32_prop.value);
			break;

		case HXB_DTYPE_FLOAT:
			value.v_float = ntohf(packet->p_float_prop.value);
			break;

		case HXB_DTYPE_128STRING:
			value.v_string = packet->p_128string_prop.value;
			break;

		case HXB_DTYPE_66BYTES:
			value.v_binary = packet->p_66bytes_prop.value;
			break;

		case HXB_DTYPE_16BYTES:
			value.v_binary = packet->p_16bytes_prop.value;
			break;

		case HXB_DTYPE_DATETIME:
			value.v_datetime = packet->p_datetime_prop.value;
		case HXB_DTYPE_TIMESTAMP:
			value.v_timestamp = uip_ntohl(packet->p_u32_prop.value);
		case HXB_DTYPE_UNDEFINED:
		default:
			syslog(LOG_ERR, "Packet of unknown datatype.");
			return HXB_ERR_DATATYPE;
	}

	return endpoint_property_write(packet->property_header.eid,
		packet->property_header.property_id, &value);
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
	syslog(LOG_NOTICE, "Received Broadcast, but no handler for datatype.");
#endif

	return HXB_ERR_SUCCESS;
}

void udp_handler_handle_incoming(struct hxb_queue_packet* R) {

	enum hxb_error_code err;
	union hxb_packet_any* packet = &(R->packet);

	// don't send error packets for broadcasts
	bool is_broadcast = packet->header.type == HXB_PTYPE_INFO
		&& uip_ipaddr_cmp(&UDP_IP_BUF->destipaddr, &hxb_group);

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

		case HXB_PTYPE_REPORT:
		case HXB_PTYPE_EPREPORT:
			err = handle_info(packet);
			break;
		case HXB_PTYPE_PINFO:
			if(uip_ipaddr_cmp(&(R->ip), &udp_master_addr)) {
				err = handle_info(packet);
			} else {
				err = HXB_ERR_UNEXPECTED_PACKET;
			}
			break;
		case HXB_PTYPE_ACK:
			err = HXB_ERR_SUCCESS;
			break;

		case HXB_PTYPE_TIMEINFO:
			{
				packet->p_timeinfo.value.year = uip_ntohs(packet->p_timeinfo.value.year);
				updateDatetime(packet->p_timeinfo.value);
				err = HXB_ERR_SUCCESS;
				break;
			}

		case HXB_PTYPE_EP_PROP_QUERY:
			{
				struct prop_cause pc = {
					.eid = uip_ntohl(packet->p_pquery.eid),
					.propid = uip_ntohl(packet->p_pquery.property_id),
					.cause_sequence_number = uip_ntohs(packet->p_query.sequence_number),
				};
				err = udp_handler_send_generated(&(R->ip), R->port, generate_epquery_response, &pc);
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
		case HXB_PTYPE_INFO:
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
			bool route_valid = rtr && rtr->isused && (rtr->isinfinite || !expired);
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
				struct hxb_queue_packet R = {
					.ip = UDP_IP_BUF->srcipaddr,
					.port = uip_ntohs(UDP_IP_BUF->srcport),
					.packet = *packet,
				};

				//ignore INFO packets
				if(!(packet->header.type == HXB_PTYPE_INFO))
					receive_packet(&R);
				else
					syslog(LOG_DEBUG, "Ignoring INFO packet!");
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
