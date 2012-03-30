#ifndef HUMIDITY_H_
#define HUMIDITY_H_

#include "contiki.h"

#define HYT321_ADDR 0x50
#define HYT321_CONV_DELAY 5

#define HYT321_HUMIDITY_LENGTH 2
#define HYT321_TEMPERATURE_LENGTH 4

float read_humidity();
float read_humidity_temp();

#endif
