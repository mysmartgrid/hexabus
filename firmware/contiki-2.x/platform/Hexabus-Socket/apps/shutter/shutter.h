#ifndef SHUTTER_H_
#define SHUTTER_H_

#include <stdbool.h>
#include "process.h"


#if defined(__AVR_ATmega1284P__)
#define SHUTTER_PORT   PORTA
#define SHUTTER_IN     PINA
#define SHUTTER_DDR    DDRA

#define SHUTTER_BUTTON_UP PA0
#define SHUTTER_BUTTON_DOWN PA1
#define SHUTTER_OUT_UP PA2
#define SHUTTER_OUT_DOWN PA3
#define SHUTTER_ENC1 PCINT4
#define SHUTTER_ENC2 PCINT5



#elif defined(__AVR_ATmega2561__)
//TODO
#else
#error Hardware not defined!
#endif

#define DEBOUNCE_TIME		   50
#define ENCODER_TIMEOUT        200


void    shutter_init(void);

void    shutter_open(void);

void    shutter_close(void);

void    shutter_open_full(void);

void    shutter_close_full(void);

void    shutter_stop(void);

bool    shutter_get_state(void);

int     shutter_get_state_int(void);

PROCESS_NAME(shutter_button_process);
PROCESS_NAME(shutter_full_process);

#endif /* SHUTTER_H_ */
