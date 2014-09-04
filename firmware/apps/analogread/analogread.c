#include "analogread.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "hexabus_config.h"
#include "value_broadcast.h"
#include "endpoints.h"
#include "endpoint_registry.h"

#define LOG_LEVEL ANALOGREAD_DEBUG
#include "syslog.h"

static float get_analogvalue(void);
static float get_lightvalue(void);

static enum hxb_error_code read_analog(struct hxb_value* value)
{
	value->v_float = get_analogvalue();
	return HXB_ERR_SUCCESS;
}

static const char ep_analogread_name[] PROGMEM = "Analog reader";
ENDPOINT_DESCRIPTOR endpoint_analogread = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_ANALOGREAD,
	.name = ep_analogread_name,
	.read = read_analog,
	.write = 0
};

ENDPOINT_PROPERTY_DESCRIPTOR prop_analogread_name = {
	.datatype = HXB_DTYPE_128STRING,
	.eid = EP_ANALOGREAD,
	.propid = EP_PROP_NAME,
};

static enum hxb_error_code read_lightsensor(struct hxb_value* value)
{
	value->v_float = get_lightvalue();
	return HXB_ERR_SUCCESS;
}

static const char ep_lightsensor_name[] PROGMEM = "Lightsensor";
ENDPOINT_DESCRIPTOR endpoint_lightsensor = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_LIGHTSENSOR,
	.name = ep_lightsensor_name,
	.read = read_lightsensor,
	.write = 0
};

ENDPOINT_PROPERTY_DESCRIPTOR prop_lightsensor_name = {
	.datatype = HXB_DTYPE_128STRING,
	.eid = EP_LIGHTSENSOR,
	.propid = EP_PROP_NAME,
};

void analogread_init() {
    ADMUX = (0<<REFS1) | (1<<REFS0); // AVCC as reference
    ADCSRA = (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2); // prescaler 128
    ADCSRA |= (1<<ADSC); // initial conversion
    ADMUX = (ADMUX & ~(ANALOGREAD_MUX_BITS)) | (ANALOGREAD_PIN & ANALOGREAD_MUX_BITS); // select pin
    ADCSRA |= (1<<ADEN); //enable ADC

#if ANALOGREAD_ENABLE
		ENDPOINT_REGISTER(endpoint_analogread);
		ENDPOINT_PROPERTY_REGISTER(prop_analogread_name);
#endif
#if LIGHTSENSOR_ENABLE
		ENDPOINT_REGISTER(endpoint_lightsensor);
		ENDPOINT_PROPERTY_REGISTER(prop_lightsensor_name);
#endif
}

static float get_analogvalue() {
    ADCSRA |= (1<<ADSC); // invoke single conversion

    while(ADCSRA & (1<<ADSC)) {} // wait for conversion to finish

    float voltage = ADCW * ANALOGREAD_MULT;

	syslog(LOG_DEBUG, "Analog value read: %ld / 1000", (uint32_t)(voltage*1000.0));

    return voltage;
}

static float get_lightvalue() {
    return get_analogvalue()/2.5;
}
