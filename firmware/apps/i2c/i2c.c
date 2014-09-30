#include "i2cmaster.h"
#include "i2c.h"
#include "hexabus_config.h"

#include <util/delay.h>

#define LOG_LEVEL I2C_DEBUG
#include "syslog.h"

uint8_t i2c_write_bytes(uint8_t address, uint8_t *data, uint8_t length) {

    if(i2c_start(address+I2C_WRITE)) {
        syslog(LOG_ERR, "I2C write failed");
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
        syslog(LOG_ERR, "I2C read failed");
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
