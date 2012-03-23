#include "humidity.h"
#include "i2c.h"
#include "hexabus_config.h"
#include "contiki.h"

#include <util/delay.h>


#if HUMIDITY_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

float read_humidity() {
    
    uint8_t bytes[2];

    if (i2c_write_bytes(0x50,NULL,0)){
        return 0;
    }

    _delay_ms(5);

    if (i2c_read_bytes(0x50,bytes,2)){
        return 0;
    }

    uint16_t rawhum = ((bytes[0]<<8 | bytes[1]) & 0x3FFF);

    PRINTF("Raw Humidity: %d\n", rawhum);

    return (100.0/16384.0*rawhum);
}

float read_humidity_temp() {
    
    uint8_t bytes[4];

    if (i2c_write_bytes(0x50,NULL,0)){
        return 0;
    }

    _delay_ms(5);

    if (i2c_read_bytes(0x50,bytes,4)){
        return 0;
    }

    uint16_t rawtemp = bytes[3]<<6 | (bytes[4]&0x3F);

    PRINTF("Raw Temperature: %d\n", rawtemp);

    return (165.0/16384.0*rawtemp)-40.0;
}
