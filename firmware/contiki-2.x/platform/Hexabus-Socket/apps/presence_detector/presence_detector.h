#ifndef PRESENCE_DETECTOR_H_
#define PRESENCE_DETECTOR_H_ 

#include <stdbool.h>
#include "process.h"

#include "hexabus_config.h"

#define ACTIVE_TIME   3
#define KEEP_ALIVE    10

void motion_detected(void);

void no_motion_detected(void);

uint8_t presence_active(void);

PROCESS_NAME(presence_detector_process);

#endif /* PRESENCE_DETECTOR_H_ */
