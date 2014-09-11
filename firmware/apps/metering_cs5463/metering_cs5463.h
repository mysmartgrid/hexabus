#ifndef APPS_METERING__CS5463_METERING__CS5463_H_F22BD763B77A6499
#define APPS_METERING__CS5463_METERING__CS5463_H_F22BD763B77A6499

#include "hexabus_config.h"

#include <stdint.h>
#include <stdbool.h>

void metering_cs5463_init(void);

// define this function to receive events from the zero cross detector
void metering_cs5463_on_zero_cross(void);

#endif
