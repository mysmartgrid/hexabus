#ifndef PRESSURE_H_
#define PRESSURE_H_

#include "contiki.h"
#include "process.h"

#define BMP085_ADDR 0xEE
#define PRESSURE_READ_DELAY 20

void pressure_init();
float read_pressure();
float read_pressure_temp();

PROCESS_NAME(pressure_process);

#endif
