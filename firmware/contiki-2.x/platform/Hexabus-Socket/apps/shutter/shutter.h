#ifndef SHUTTER_H_
#define SHUTTER_H_

#include <stdbool.h>
#include "process.h"

#include "hexabus_config.h"

#if defined(__AVR_ATmega1284P__)
#define SHUTTER_PORT   PORTA
#define SHUTTER_IN     PINA
#define SHUTTER_DDR    DDRA

#define SHUTTER_OUT_UP PA2
#define SHUTTER_OUT_DOWN PA3
#define SHUTTER_ENC1 PCINT4
#define SHUTTER_ENC2 PCINT5



#elif defined(__AVR_ATmega2561__)
//TODO
#else
#error Hardware not defined!
#endif

#define ENCODER_TIMEOUT        300UL


void    shutter_init(void);

void    shutter_toggle(uint8_t);

void    shutter_set(uint8_t);

void    shutter_stop(void);

uint8_t     shutter_get_state(void);

PROCESS_NAME(shutter_process);

#endif /* SHUTTER_H_ */
