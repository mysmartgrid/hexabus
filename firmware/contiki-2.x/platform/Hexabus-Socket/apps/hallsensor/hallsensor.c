#include "hallsensor.h"
//#include <util/delay.h>
//#include "sys/clock.h"
//#include "sys/etimer.h" //contiki event timer library
//#include "contiki.h"
//#include "hexabus_config.h"
//#include "value_broadcast.h"
#include "endpoints.h"
#include "endpoint_registry.h"

#define LOG_LEVEL HALLSENSOR_DEBUG
#include "syslog.h"

/*------------------------------------------------------*/
PROCESS_NAME(hallsensor_process);
PROCESS(hallsensor_process, "Hallsensor Process");
//AUTOSTART_PROCESSES(&hallsensor_process);
/*------------------------------------------------------*/

uint8_t hallsensor_is_running()
{
	return (process_is_running(&hallsensor_process));
}

void hallsensor_start()
{
	if ( hallsensor_is_running() )
	{
		syslog(LOG_DEBUG, "Hallsensor already running!");
	} else {
		syslog(LOG_DEBUG, "Starting hallsensor.");
		process_start(&hallsensor_process, NULL);
	}
}

void hallsensor_stop()
{
	if ( !hallsensor_is_running() )
	{
		syslog(LOG_DEBUG, "Hallsensor not running!");
	} else {
		syslog(LOG_DEBUG, "Stopping hallsensor.");
		process_exti(&hallsensor_process);
	}
}

static enum hxb_error_code read_hallsensor(struct hxb_value* value)
{
	//value->v_float = get_analogvalue();
	return HXB_ERR_SUCCESS;
}

static const char ep_hallsensor_name[] PROGMEM = "Hallsensor metering";
ENDPOINT_DESCRIPTOR endpoint_hallsensor = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_POWER_METER,
	.name = ep_hallsensor_name,
	.read = read_hallsensor,
	.write = 0
};

void power_init() {
	//TODO: do we want continuous operation of the adc?
	ADMUX = (0<<REFS1) | (1<<REFS0); // AVCC as reference
	ADCSRA = (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2); // prescaler 128
	ADCSRA |= (1<<ADSC); // initial conversion
	ADMUX = (ADMUX & ~(HALLSENSOR_MUX_BITS)) | (HALLSENSOR_PIN & HALLSENSOR_MUX_BITS); // select pin
	ADCSRA |= (1<<ADEN); //enable ADC

#if HALLSENSOR_ENABLE
	ENDPOINT_REGISTER(endpoint_hallsensor);
#endif
#if HALLSENSOR_FAULT_ENABLE
	DDRC |= (1<<HALLSENSOR_FAULT_EN); // PC6 (FAULT_EN) as output
	PORTC |= (1<<HALLSENSOR_FAULT_EN); // set PC6 high to enable fault detection
	DDRC &= ~(1<<HALLSENSOR_FAULT); // PC7 (FAULT) as input
#endif

	//hallsensor_start();
}

PROCESS_THREAD(hallsensor_process, ev, data)
{
	PROCESS_BEGIN();

	//initialize timer
	static struct etimer hallsensor_timer;
	etimer_set(&hallsensor_timer, CLOCK_SECOND); //TODO: we need a much higher resolution here
	//TODO: initialize array of measurements
	while(hallsensor_is_running())
	{
		PROCESS_WAIT_EVENT();
		if (!hallsensor_is_running())
			break;

		syslog(LOG_DEBUG, "Hallsensor received event %d\n", ev);
		if ( ev == PROCESS_TIMER_EVENT )
		{
			etimer_reset(&hallsensor_timer);
			//TODO: read analog value and update current value in array
		}
	}
	PROCESS_END();
}

/*static float get_analogvalue(void);
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
}*/
