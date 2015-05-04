#include "humidity.h"
#include "i2c.h"
#include "hexabus_config.h"
#include "contiki.h"

#include "endpoint_registry.h"
#include "endpoints.h"

#include <util/delay.h>


#define LOG_LEVEL HUMIDITY_DEBUG
#include "syslog.h"

#include "health.h"

char read_humidity(float* target)
{
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

	syslog(LOG_DEBUG, "Raw Humidity: %d", rawhum);

	*target = (100.0/16384.0*rawhum);
	return 1;
}

char read_humidity_temp(float* target)
{
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

	syslog(LOG_DEBUG, "Raw Temperature: %d", rawtemp);

	*target = (165.0/16384.0*rawtemp)-40.0;
	return 1;
}

static enum hxb_error_code read(struct hxb_value* value)
{
	if (read_humidity(&value->v_float)) {
		health_update(HE_HYT_BROKEN, 0);
		return HXB_ERR_SUCCESS;
	} else {
		health_update(HE_HYT_BROKEN, 1);
		return HXB_ERR_NO_VALUE;
	}
}

void humidity_init(void)
{
	ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_HUMIDITY, "Humidity sensor", read, 0);
}
