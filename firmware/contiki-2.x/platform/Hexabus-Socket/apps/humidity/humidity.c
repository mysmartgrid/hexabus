#include "humidity.h"
#include "i2c.h"
#include "hexabus_config.h"
#include "contiki.h"

#include "endpoint_registry.h"
#include "endpoints.h"

#include <util/delay.h>


#if HUMIDITY_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

float read_humidity() {

    uint8_t bytes[HYT321_HUMIDITY_LENGTH];

    //Start conversion
    if (i2c_write_bytes(HYT321_ADDR,NULL,0)){
        return 0;
    }

    _delay_ms(HYT321_CONV_DELAY);

    if (i2c_read_bytes(HYT321_ADDR,bytes,HYT321_HUMIDITY_LENGTH)){
        return 0;
    }

    //Convert to %r.h. (refer datasheet)

    uint16_t rawhum = ((bytes[0]<<8 | bytes[1]) & 0x3FFF);

    PRINTF("Raw Humidity: %d\n", rawhum);

    return (100.0/16384.0*rawhum);
}

float read_humidity_temp() {

    uint8_t bytes[HYT321_TEMPERATURE_LENGTH];

    if (i2c_write_bytes(HYT321_ADDR,NULL,0)){
        return 0;
    }

    _delay_ms(HYT321_CONV_DELAY);

    if (i2c_read_bytes(HYT321_ADDR,bytes,HYT321_TEMPERATURE_LENGTH)){
        return 0;
    }

    //Convert to degrees celcius (refer datasheet)

    uint16_t rawtemp = bytes[2]<<6 | (bytes[3]&0x3F);

    PRINTF("Raw Temperature: %d\n", rawtemp);

    return (165.0/16384.0*rawtemp)-40.0;
}

static enum hxb_error_code read(uint32_t eid, struct hxb_value* value)
{
	value->v_float = read_humidity();
	return HXB_ERR_SUCCESS;
}

static const char ep_name[] PROGMEM = "Humidity sensor";
ENDPOINT_DESCRIPTOR endpoint_humidity = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_HUMIDITY,
	.name = ep_name,
	.read = read,
	.write = 0
};

void humidity_init(void)
{
	ENDPOINT_REGISTER(endpoint_humidity);
}
