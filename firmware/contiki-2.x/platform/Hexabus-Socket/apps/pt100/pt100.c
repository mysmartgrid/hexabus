#include "pt100.h"
#include "endpoint_registry.h"
#include "endpoints.h"
#include "hexabus_config.h"

#define LOG_LEVEL PT100_DEBUG
#include "syslog.h"

#define SUPERSAMPLING_MAX_ITERATION 8

static uint16_t adc_get_single_sample(uint8_t channel);

// Taken from 
// http://www.mikrocontroller.net/articles/AVR-GCC-Tutorial/Analoge_Ein-_und_Ausgabe
static void adc_init(void)
{
	// interne Referenzspannung (1.1V) als Referenz für den ADC wählen,
	// siehe m1284p datasheet S. 255
	ADMUX = (1<<REFS1) | (0<<REFS0);

	// Bit ADFR ("free running") in ADCSRA steht beim Einschalten
	// schon auf 0, also single conversion

	// ADC Prescaler Calculation: We run at 8 MHz, the ADC needs a frequency 
	// between 0.05 MHz and 0.2 MHz:
	// 0.05 <= 8/x <= 0.2
	// <=> 0.05*x <= 8 <= 0.2*x
	// Satisfied for x=128. Set Prescaler accordingly:
	ADCSRA =  (1 << ADPS2) | (1<<ADPS1) | (1<<ADPS0);     
	ADCSRA |= (1<<ADEN);                  // ADC aktivieren

	/* ADCW muss einmal gelesen werden, sonst wird Ergebnis der nächsten
	   Wandlung nicht übernommen. */
	adc_get_single_sample(0);
}

static uint16_t adc_get_single_sample(uint8_t channel)
{
	// Kanal waehlen, ohne andere Bits zu beeinflußen
	ADMUX = (ADMUX & ~(0x1F)) | (channel & 0x1F);
	ADCSRA |= (1<<ADSC);                  // eine ADC-Wandlung starten
	while (ADCSRA & (1<<ADSC)) {         // auf Abschluss der Konvertierung warten
	}
	return ADCW;
}

static uint16_t adc_get_super_sample(uint8_t channel)
{
	uint16_t buffer = 0;
	// no overflow here: 8*1024 <<<< max(uint16_t)
	for (uint8_t i = 0; i < SUPERSAMPLING_MAX_ITERATION; i++) {
		buffer += adc_get_single_sample(channel);
	}
	return buffer / SUPERSAMPLING_MAX_ITERATION;
}

static float temperature_adc(uint8_t channel)
{
	uint16_t adc = adc_get_super_sample(channel);
	return PT100_TEMP_MIN + (PT100_TEMP_MAX - PT100_TEMP_MIN) / (PT100_ADC_MAX - PT100_ADC_MIN) * (float)adc;
}

static enum hxb_error_code read_hot(struct hxb_value* value)
{
	value->v_float = temperature_adc(PT100_CHANNEL_HOT);
	return HXB_ERR_SUCCESS;
}

static enum hxb_error_code read_cold(struct hxb_value* value)
{
	value->v_float = temperature_adc(PT100_CHANNEL_COLD);
	return HXB_ERR_SUCCESS;
}

static const char ep_hot[] PROGMEM = "Heater inflow temperature";
ENDPOINT_DESCRIPTOR endpoint_heater_hot = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_HEATER_HOT,
	.name = ep_hot,
	.read = read_hot,
	.write = 0
};

static const char ep_cold[] PROGMEM = "Heater outflow temperature";
ENDPOINT_DESCRIPTOR endpoint_heater_cold = {
	.datatype = HXB_DTYPE_FLOAT,
	.eid = EP_HEATER_COLD,
	.name = ep_cold,
	.read = read_cold,
	.write = 0
};

void pt100_init()
{
	ENDPOINT_REGISTER(endpoint_heater_hot);
	ENDPOINT_REGISTER(endpoint_heater_cold);

	adc_init();
}
