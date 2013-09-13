#include "hallsensor.h"
#include "hexabus_config.h"
#include "endpoints.h"
#include "endpoint_registry.h"
#include <avr/interrupt.h>
#include <stdlib.h>

#define LOG_LEVEL HALLSENSOR_DEBUG
#include "syslog.h"

static uint16_t samples[96];
static float values[50];
static uint8_t samplepos;
static uint8_t valuepos;
volatile static float average;

static enum hxb_error_code read_hallsensor(struct hxb_value* value)
{
	syslog(LOG_DEBUG, "Reading hallsensor value");
	//Calculate the mean power for one second to reduce noise
	float power = 0;
	for ( uint8_t i = 0; i < 50; i++ )
	{
		power += values[i];
	}
	value->v_float = power/50.0;

	return HXB_ERR_SUCCESS;
}

static const char ep_hallsensor_name[] PROGMEM = "Hallsensor metering";
ENDPOINT_DESCRIPTOR endpoint_hallsensor = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_HALLSENSOR,
	.name = ep_hallsensor_name,
	.read = read_hallsensor,
	.write = 0
};

void hallsensor_init() {
#if HALLSENSOR_ENABLE
	ADMUX = (0<<REFS1) | (1<<REFS0); // AVCC as reference
	ADCSRA = (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2); // prescaler 128
	ADCSRA |= (1<<ADATE) | (1<<ADIE); // free running adc with interrupt enabled
	ADCSRB &= ~((1<<ADTS2) | (1<<ADTS1) | (1<<ADTS0));
	ADMUX = (ADMUX & ~(HALLSENSOR_MUX_BITS)) | (HALLSENSOR_PIN & HALLSENSOR_MUX_BITS); // select pin
	ADCSRA |= (1<<ADEN); //enable ADC
	ADCSRA |= (1<<ADSC); //start first conversion

	ENDPOINT_REGISTER(endpoint_hallsensor);

	samplepos = 0;
	valuepos = 0;
#endif
#if HALLSENSOR_FAULT_ENABLE
	DDRC |= (1<<HALLSENSOR_FAULT_EN); // PC6 (FAULT_EN) as output
	PORTC |= (1<<HALLSENSOR_FAULT_EN); // set PC6 high to enable fault detection
	DDRC &= ~(1<<HALLSENSOR_FAULT); // PC7 (FAULT) as input
#endif
}

//Assumption: This interrupt is executed 96 per mains sine
//This equals 4000 times per second
ISR(ADC_vect)
{
	samples[samplepos++] = ADCW;
	if ( samplepos >= 96 )
	{
		// We have sampled one mains sine, so we reset the counter and calculate the absolute average
		samplepos = 0;

		uint16_t mean = 0;
		for ( uint8_t i = 0; i < 96; i++ )
		{
			//Compensate for the offset of Vcc/2 and add up the absolute values
			mean += abs(samples[i] - 512);
		}
		uint32_t v = ((uint32_t)mean)*322;
		values[valuepos++] = ((float)v)/1848.0/96.0*230.0;

		if ( valuepos >= 50 )
			valuepos = 0;
	}
}

