#ifndef VALUE_BROADCAST_H_
#define VALUE_BROADCAST_H_

#include "process.h"

PROCESS_NAME(value_broadcast_process);

void broadcast_value(uint32_t eid);
void init_value_broadcast(void);

#endif
