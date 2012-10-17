#include "analogread.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "hexabus_config.h"
#include "value_broadcast.h"

#if ANALOGREAD_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

void analogread_init() {
    ADMUX = (0<<REFS1) | (1<<REFS0); // AVCC as reference
    ADCSRA = (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2); // prescaler 128
    ADCSRA |= (1<<ADSC); // initial conversion
    ADMUX = (ADMUX & ~(ANALOGREAD_MUX_BITS)) | (ANALOGREAD_PIN & ANALOGREAD_MUX_BITS); // select pin
    ADCSRA |= (1<<ADEN); //enable ADC
}

float get_analogvalue() {
    ADCSRA |= (1<<ADSC); // invoke single conversion

    while(ADCSRA & (1<<ADSC)) {} // wait for conversion to finish

    float voltage = ADCW * ANALOGREAD_MULT;

    PRINTF("Analog value read: %d / 1000\n", (uint32_t)(voltage*1000.0));

    return voltage;
}

float get_lightvalue() {
    return get_analogvalue()/2.5;
}
