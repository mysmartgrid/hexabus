#include "i2cmaster.h"
#include "i2c.h"
#include "hexabus_config.h"

#include <util/delay.h>

#if I2C_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

uint8_t i2c_write_bytes(uint8_t address, uint8_t *data, uint8_t length) {

    if(i2c_start(address+I2C_WRITE)) {
        PRINTF("I2C write failed\n");
        return 1;
    }


    uint8_t i;

    for(i=0;i<length;i++) {
        i2c_write(data[i]);
    }

    i2c_stop();

    return 0;
}

uint8_t i2c_read_bytes(uint8_t address, uint8_t *data, uint8_t length) {

    if((length<1)||(i2c_start(address+I2C_READ))) {
        PRINTF("I2C read failed\n");
        return 1;
    }

    uint8_t i;

    for(i=0;i<length-1;i++) {
        data[i] = i2c_readAck();
    }
    data[length-1] = i2c_readNak();

    i2c_stop();

    return 0;
}
