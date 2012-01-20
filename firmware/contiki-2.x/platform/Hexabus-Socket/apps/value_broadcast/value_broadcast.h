#ifndef VALUE_BROADCAST_H_
#define VALUE_BROADCAST_H_

#include "process.h"

#if VALUE_BROADCAST_AUTO_INTERVAL
PROCESS_NAME(value_broadcast_process);
#endif

void broadcast_value(uint8_t eid);
void init_value_broadcast(void);

#endif
