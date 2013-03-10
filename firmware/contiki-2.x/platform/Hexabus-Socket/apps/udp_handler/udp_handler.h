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
#include "../../../../shared/hexabus_packet.h"

extern process_event_t sm_data_received_event;

PROCESS_NAME(udp_handler_process);

enum hxb_error_code udp_handler_send_generated(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code (*packet_gen_fn)(union hxb_packet_any* buffer, void* data), void* data);

enum hxb_error_code udp_handler_send_error(const uip_ipaddr_t* toaddr, uint16_t toport, enum hxb_error_code code);

#endif /* UDP_HANDLER_H_ */
