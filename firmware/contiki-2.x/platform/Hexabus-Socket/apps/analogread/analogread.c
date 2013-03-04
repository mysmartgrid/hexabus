#include "analogread.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "hexabus_config.h"
#include "value_broadcast.h"
#include "endpoints.h"
#include "endpoint_registry.h"

#if ANALOGREAD_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static float get_analogvalue(void);
static float get_lightvalue(void);

static enum hxb_error_code read_analog(uint32_t eid, struct hxb_value* value)
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

static enum hxb_error_code read_lightsensor(uint32_t eid, struct hxb_value* value)
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

void analogread_init() {
    ADMUX = (0<<REFS1) | (1<<REFS0); // AVCC as reference
    ADCSRA = (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2); // prescaler 128
    ADCSRA |= (1<<ADSC); // initial conversion
    ADMUX = (ADMUX & ~(ANALOGREAD_MUX_BITS)) | (ANALOGREAD_PIN & ANALOGREAD_MUX_BITS); // select pin
    ADCSRA |= (1<<ADEN); //enable ADC

#if ANALOGREAD_ENABLE
		ENDPOINT_REGISTER(endpoint_analogread);
#endif
#if LIGHTSENSOR_ENABLE
		ENDPOINT_REGISTER(endpoint_lightsensor);
#endif
}

static float get_analogvalue() {
    ADCSRA |= (1<<ADSC); // invoke single conversion

    while(ADCSRA & (1<<ADSC)) {} // wait for conversion to finish

    float voltage = ADCW * ANALOGREAD_MULT;

    PRINTF("Analog value read: %ld / 1000\n", (uint32_t)(voltage*1000.0));

    return voltage;
}

static float get_lightvalue() {
    return get_analogvalue()/2.5;
}
