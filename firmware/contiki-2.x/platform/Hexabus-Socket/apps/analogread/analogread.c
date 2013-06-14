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

static void acs709_reset_fault()
{
	PORTC &= ~(1<<PC6); // set PC6 low to reset fault detection
	//TODO: wait for >= 500ns
	PORTC |= (1<<PC6);
};

static enum hxb_error_code read_acs709(struct hxb_value* value)
{
	if ( PINC & (1<<PC7) ) {
		value->v_float = get_analogvalue();
		return HXB_ERR_SUCCESS;
	} else {
		acs709_reset_fault();
	}
	return HXB_ERR_INTERNAL;
}

static const char ep_acs709_name[] PROGMEM = "ACS709 power meter";
ENDPOINT_DESCRIPTOR endpoint_acs709 = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_ACS709,
	.name = ep_acs709_name,
	.read = read_acs709,
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
#if ACS709_ENABLE
		DDRC |= (1<<PC6); // PC6 (FAULT_EN) as output
		PORTC |= (1<<PC6); // set PC6 high to enable fault detection
		DDRC &= ~(1<<PC7); // PC7 (FAULT) as input
		ENDPOINT_REGISTER(endpoint_acs709);
#endif
}

static float get_analogvalue() {
    ADCSRA |= (1<<ADSC); // invoke single conversion

    while(ADCSRA & (1<<ADSC)) {} // wait for conversion to finish

#if ANOLGREAD_ENABLE
    float voltage = ADCW * ANALOGREAD_MULT;
#endif
#if ACS709_ENABLE
    float voltage = ADCW * ACS709_MULT;
#endif

	syslog(LOG_DEBUG, "Analog value read: %ld / 1000", (uint32_t)(voltage*1000.0));

    return voltage;
}

static float get_lightvalue() {
    return get_analogvalue()/2.5;
}
