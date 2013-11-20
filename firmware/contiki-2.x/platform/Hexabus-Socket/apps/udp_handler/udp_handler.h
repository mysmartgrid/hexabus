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

extern process_event_t udp_handler_event;

enum hxb_error_code udp_handler_send_generated(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code (*packet_gen_fn)(union hxb_packet_any* buffer, void* data), void* data);

enum hxb_error_code udp_handler_send_error(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code code, uint16_t cause_sequnence_number);

#endif /* UDP_HANDLER_H_ */
