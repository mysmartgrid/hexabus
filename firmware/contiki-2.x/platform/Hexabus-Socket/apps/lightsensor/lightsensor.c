#include "lightsensor.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "hexabus_config.h"
#include "value_broadcast.h"

#include <stdio.h>
void lightsensor_init() {
    ADMUX = (0<<REFS1) | (1<<REFS0); // AVCC as reference
    ADCSRA = (1<<ADPS0) | (1<<ADPS2); // prescaler 128
    ADCSRA |= (1<<ADSC); // initial conversion
    ADMUX = (ADMUX & ~(0x1F)) | (LIGHTSENSOR_PIN & 0x1F); // select pin
    ADCSRA |= (1<<ADEN); //enable ADC
}

uint32_t get_lightvalue() {
    ADCSRA |= (1<<ADSC); // invoke single conversion
    
    while(ADCSRA & (1<<ADSC)) {} // wait for conversion to finish

    return ADCW;
}
