
#include "contiki-conf.h"
#include "dev/leds.h"
#include <avr/io.h>

/* LED ports */
#if defined (__AVR_ATmega2561__)
#define LEDS_PxDIR 			DDRG // port direction register
#define LEDS_PxOUT 			PORTG // port register

#define LEDS_CONF_RED    	(1<<PG0) //red led
#define LEDS_CONF_GREEN  	(1<<PG1) //green led

#elif defined (__AVR_ATmega1284P__)

#define LEDS_PxDIR 			DDRC // port direction register
#define LEDS_PxOUT 			PORTC // port register


#define LEDS_CONF_RED    	(1<<PC0) //red led
#define LEDS_CONF_GREEN  	(1<<PC1) //green led

#endif

/* LEDs are enabled with 0 and disabled with 1. So we have to toggle the
 * values of LED_ON and LED_OFF in leds.h
 * */

void leds_arch_init(void)
{
  LEDS_PxDIR |= (LEDS_CONF_RED | LEDS_CONF_GREEN);
  LEDS_PxOUT |= (LEDS_CONF_RED | LEDS_CONF_GREEN);
}

unsigned char leds_arch_get(void)
{
  return ((LEDS_PxOUT & LEDS_CONF_RED) ? 0 : LEDS_RED)
    | ((LEDS_PxOUT & LEDS_CONF_GREEN) ? 0 : LEDS_GREEN);
}

void leds_arch_set(unsigned char leds)
{
  LEDS_PxOUT = (LEDS_PxOUT & ~(LEDS_CONF_RED|LEDS_CONF_GREEN))
    | ((leds & LEDS_RED) ? 0 : LEDS_CONF_RED)
    | ((leds & LEDS_GREEN) ? 0 : LEDS_CONF_GREEN);
}
