#ifndef PRESSURE_H_
#define PRESSURE_H_

#include "contiki.h"
#include "process.h"

#define BMP085_ADDR 0xEE

#define AC1_ADDR 0xAA
#define AC2_ADDR 0xAC
#define AC3_ADDR 0xAE
#define AC4_ADDR 0xB0
#define AC5_ADDR 0xB2
#define AC6_ADDR 0xB4
#define B1_ADDR 0xB6
#define B2_ADDR 0xB8
#define MB_ADDR 0xBA
#define MC_ADDR 0xBC
#define MD_ADDR 0xBE

#define BMP085_CONTROL 0xF4
#define BMP085_VALUE 0xF6
#define BMP085_VALUE_XLSB 0xF8
#define BMP085_CONV_TEMPERATURE 0x2E
#define BMP085_CONV_PRESSURE 0x34

#define PRESSURE_READ_DELAY 30UL
#define PRESSURE_READ_TEMP_DELAY 10UL

void pressure_init();
float read_pressure();
float read_pressure_temp();

PROCESS_NAME(pressure_process);

#endif
