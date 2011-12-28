#ifndef HEXAPRESSURE_H_
#define HEXAPRESSURE_H_ 

#include <stdbool.h>
#include "process.h"

#include "hexabus_config.h"

#define ACTIVE_TIME   3

void motion_detected(void);

void no_motion_detected(void);

uint8_t presence_active(void);

PROCESS_NAME(hexapressure_process);

#endif /* HEXAPRESSURE_H_ */
