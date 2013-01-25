#ifndef VALUE_BROADCAST_H_
#define VALUE_BROADCAST_H_

#include "process.h"

// TODO if value_broadcast_auto_interval || metering_immediate_bcast
// #if VALUE_BROADCAST_AUTO_INTERVAL
PROCESS_NAME(value_broadcast_process);
// #endif

void broadcast_value(uint32_t eid);
void init_value_broadcast(void);
void resend_broadcast_packet(void* data, int* length);
void send_resend_request(uint32_t seqnum);

extern process_event_t immediate_broadcast_event;

#endif
