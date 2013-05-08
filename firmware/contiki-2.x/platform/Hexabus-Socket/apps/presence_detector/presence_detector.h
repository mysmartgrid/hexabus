#ifndef PRESENCE_DETECTOR_H_
#define PRESENCE_DETECTOR_H_ 

#include <stdbool.h>
#include "process.h"

#include "hexabus_config.h"

#define PRESENCE 1
#define NO_PRESENCE 0

uint8_t is_presence(void);
void global_presence_detected(void);
void raw_presence_detected(void); 
void no_raw_presence(void);
void presence_detector_init();

PROCESS_NAME(presence_detector_process);

#endif /* PRESENCE_DETECTOR_H_ */
