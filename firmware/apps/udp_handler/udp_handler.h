/*
 * udp_handler.h
 *
 *  Created on: 12.05.2011
 *      Author: neda.petreska
 */

#ifndef UDP_HANDLER_H_
#define UDP_HANDLER_H_

#include "process.h"
#include <stddef.h>
#include "net/uip.h"
#include "hexabus_packet.h"

PROCESS_NAME(udp_handler_process);

typedef enum {
	UDP_HANDLER_UP,
	UDP_HANDLER_DOWN
} udp_handler_event_t;

struct eid_cause {
	uint32_t eid;
	uint16_t cause_sequence_number;
};

struct prop_cause {
	uint32_t eid;
	uint16_t propid;
	uint16_t cause_sequence_number;
};

extern process_event_t udp_handler_event;
uip_ipaddr_t udp_master_addr;

enum hxb_error_code udp_handler_send_generated(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code (*packet_gen_fn)(union hxb_packet_any* buffer, void* data), void* data);
enum hxb_error_code udp_handler_send_generated_reliable(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code (*packet_gen_fn)(union hxb_packet_any* buffer, void* data), void* data);

enum hxb_error_code udp_handler_send_error(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code code, uint16_t cause_sequence_number);

enum hxb_error_code udp_handler_send_ack(const uip_ipaddr_t* toaddr, uint16_t toport, uint16_t cause_sequence_number);

void do_udp_send(const uip_ipaddr_t* toaddr, uint16_t toport, union hxb_packet_any* packet);

void udp_handler_handle_incoming(struct hxb_queue_packet* R);

#endif /* UDP_HANDLER_H_ */
