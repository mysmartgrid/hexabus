#ifndef HEXAPRESSURE_H_
#define HEXAPRESSURE_H_ 

#include <stdbool.h>
#include "process.h"

#include "hexabus_config.h"

#if defined(__AVR_ATmega1284P__)
#define HEXAPRESSURE_PORT   PORTA
#define HEXAPRESSURE_IN     PINA
#define HEXAPRESSURE_DDR    DDRA
#define SCL                 PA0
#define SDA                 PA1
#else
#error Hardware not defined!
#endif

void hexapressure_init(void);

PROCESS_NAME(hexapressure_process);

#endif /* HEXAPRESSURE_H_ */
