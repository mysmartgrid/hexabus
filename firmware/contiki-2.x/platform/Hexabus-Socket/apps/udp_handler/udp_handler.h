/*
 * udp_handler.h
 *
 *  Created on: 12.05.2011
 *      Author: neda.petreska
 */

#ifndef UDP_HANDLER_H_
#define UDP_HANDLER_H_

#include "process.h"

extern process_event_t sm_data_received_event;

PROCESS_NAME(udp_handler_process);

#endif /* UDP_HANDLER_H_ */
