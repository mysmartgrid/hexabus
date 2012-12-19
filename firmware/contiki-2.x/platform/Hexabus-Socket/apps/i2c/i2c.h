#ifndef I2C_H_
#define I2C_H_

#include "contiki.h"

uint8_t i2c_write_bytes(uint8_t address, uint8_t *data, uint8_t length);
uint8_t i2c_read_bytes(uint8_t address, uint8_t *data, uint8_t length);

#endif
