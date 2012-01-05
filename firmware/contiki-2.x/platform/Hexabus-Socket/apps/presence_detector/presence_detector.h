#ifndef PRESENCE_DETECTOR_H_
#define PRESENCE_DETECTOR_H_ 

#include <stdbool.h>
#include "process.h"

#include "hexabus_config.h"

void presence_detected(void);

void no_presence_detected(void);

uint8_t is_presence(void);

PROCESS_NAME(presence_detector_process);

#endif /* PRESENCE_DETECTOR_H_ */
