#ifndef HEXAPUSH_H_
#define HEXAPUSH_H_

#include <stdbool.h>
#include "process.h"


#if defined(__AVR_ATmega1284P__)
#define HEXAPUSH_PORT   PORTA
#define HEXAPUSH_IN     PINA
#define HEXAPUSH_DDR    DDRA

//Comment unneeded buttons
#define HEXAPUSH_B1     PA0
#define HEXAPUSH_B2     PA1
//#define HEXAPUSH_B3     PA2
//#define HEXAPUSH_B4     PA3
//#define HEXAPUSH_B5     PA4
//#define HEXAPUSH_B6     PA5
//#define HEXAPUSH_B7     PA6
//#define HEXAPUSH_B8     PA7

#elif defined(__AVR_ATmega2561__)
//TODO?
#else
#error Hardware not defined!
#endif

#define DEBOUNCE_TIME		   50UL
#define LONG_CLICK_MULT        6

void hexapush_init(void);

uint8_t get_buttonstate();

PROCESS_NAME(hexapush_process);

#endif /* HEXAPUSH_H_ */
