#ifndef VALUE_BROADCAST_H_
#define VALUE_BROADCAST_H_

#include "process.h"

// TODO if value_broadcast_auto_interval || metering_immediate_bcast
// #if VALUE_BROADCAST_AUTO_INTERVAL
PROCESS_NAME(value_broadcast_process);
// #endif

void broadcast_value(uint32_t eid);
void broadcast_value_ptr(void* eidp);
void init_value_broadcast(void);

extern process_event_t immediate_broadcast_event;

#endif
