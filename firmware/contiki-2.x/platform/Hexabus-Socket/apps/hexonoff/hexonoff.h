#ifndef HEXONOFF_H_
#define HEXONOFF_H_

#include <stdbool.h>
#include "process.h"


#if defined(__AVR_ATmega1284P__)
#define HEXONOFF_PORT   PORTA
#define HEXONOFF_IN     PINA
#define HEXONOFF_DDR    DDRA

//Comment unneeded outputs
//#define HEXONOFF_OUT0     PA0
//#define HEXONOFF_OUT1     PA1
#define HEXONOFF_OUT2     PA2
#define HEXONOFF_OUT3     PA3
//#define HEXONOFF_OUT4     PA4
//#define HEXONOFF_OUT5     PA5
//#define HEXONOFF_OUT6     PA6
//#define HEXONOFF_OUT7     PA7

#elif defined(__AVR_ATmega2561__)
//TODO?
#else
#error Hardware not defined!
#endif

void hexonoff_init(void);

void set_outputs(uint8_t o_vec);
void toggle_outputs(uint8_t o_vec);
uint8_t get_outputs();

#endif /* HEXONOFF_H_ */
