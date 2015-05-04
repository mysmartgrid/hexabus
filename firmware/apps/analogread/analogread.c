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

static enum hxb_error_code read_lightsensor(struct hxb_value* value)
{
	value->v_float = get_lightvalue();
	return HXB_ERR_SUCCESS;
}

void analogread_init() {
    ADMUX = (0<<REFS1) | (1<<REFS0); // AVCC as reference
    ADCSRA = (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2); // prescaler 128
    ADCSRA |= (1<<ADSC); // initial conversion
    ADMUX = (ADMUX & ~(ANALOGREAD_MUX_BITS)) | (ANALOGREAD_PIN & ANALOGREAD_MUX_BITS); // select pin
    ADCSRA |= (1<<ADEN); //enable ADC

#if ANALOGREAD_ENABLE
		ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_ANALOGREAD, "Analog reader", read_analog, 0);
#endif
#if LIGHTSENSOR_ENABLE
		ENDPOINT_REGISTER(HXB_DTYPE_FLOAT, EP_LIGHTSENSOR, "Lightsensor", read_lightsensor, 0);
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
