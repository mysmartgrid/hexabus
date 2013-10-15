#include "hallsensor.h"
#include "hexabus_config.h"
#include "endpoints.h"
#include "endpoint_registry.h"
#include "value_broadcast.h"
#include <avr/eeprom.h>
#include "eeprom_variables.h"
#include <avr/interrupt.h>
#include <stdlib.h>

#define LOG_LEVEL HALLSENSOR_DEBUG
#include "syslog.h"

#define HALLSENSOR_SAMPLES_PER_SECOND 4500
#define HALLSENSOR_SAMPLES_PER_SINE 90

volatile static uint16_t samplesum; // 10bit ADC -512 -> 9bit ADC * 96 -> 16bit
volatile static uint16_t valuesum; // 9bit averaged ADC value
static float average;
static uint8_t samplepos;
static uint8_t valuepos;
static float calibration_value;
static float hallsensor_calibration;

static enum hxb_error_code read_hallsensor(struct hxb_value* value)
{
	syslog(LOG_DEBUG, "Reading hallsensor value");
	value->v_float = average;

	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code calibrate_hallsensor(const struct hxb_envelope* env) {
	syslog(LOG_DEBUG, "Calibrating for %lu", (uint32_t) env->value.v_float);
	hallsensor_calibration = env->value.v_float;

	return HXB_ERR_SUCCESS;
}

static const char ep_hallsensor_name[] PROGMEM = "Hallsensor metering";
ENDPOINT_DESCRIPTOR endpoint_hallsensor = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_HALLSENSOR,
	.name = ep_hallsensor_name,
	.read = read_hallsensor,
	.write = calibrate_hallsensor
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
	samplesum = 0;
	valuesum = 0;
	average = 0.0;
	hallsensor_calibration = 0;
	calibration_value = eeprom_read_float((float*) EE_HALLSENSOR_CAL);
#endif
#if HALLSENSOR_FAULT_ENABLE
	DDRC |= (1<<HALLSENSOR_FAULT_EN); // PC6 (FAULT_EN) as output
	PORTC |= (1<<HALLSENSOR_FAULT_EN); // set PC6 high to enable fault detection
	DDRC &= ~(1<<HALLSENSOR_FAULT); // PC7 (FAULT) as input
#endif
}

//Assumption: This interrupt is executed HALLSENSOR_SAMPLES_PER_SINE times per mains sine
ISR(ADC_vect)
{
	//Compensate for the offset of Vcc/2 and add up the absolute values
	samplesum += abs(ADCW - 512);
	if ( ++samplepos >= HALLSENSOR_SAMPLES_PER_SINE )
	{
		samplepos = 0;
		//syslog(LOG_DEBUG, "calculated sine value: %u", samplesum);
		valuesum += samplesum;
		samplesum = 0;
		if ( ++valuepos >= 50 ) // mains frequency of 50Hz -> 50 sines per second
		{
			syslog(LOG_DEBUG, "calculated seconds value: %u", valuesum);
			valuepos = 0;
			if ( hallsensor_calibration > 0 )
			{
				calibration_value = valuesum/hallsensor_calibration;
				eeprom_update_float((float*) EE_HALLSENSOR_CAL, calibration_value);
				valuesum = 0;
				syslog(LOG_DEBUG, "new calibration factor: %u", (uint32_t) (calibration_value*100.0));
				hallsensor_calibration = 0;
			} else {
				// 10bit ADC with AREF = 3.3V as reference -> 3.22 mV resolution
				// ACS709 datasheet: 28mV/A at 5V Vcc -> 28*3.3/5 = 18.48mv/A at 3.3V Vcc
				// 230V mains voltage
				//average = ((float)((valuesum/50)*322))/1848.0/90.0*230.0;
				average = ((float)valuesum)/calibration_value;
				valuesum = 0;
				broadcast_value(EP_HALLSENSOR);
			}
		}
	}
}

