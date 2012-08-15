#ifndef ANALOGREAD_H_
#define ANALOGREAD_H_ 

#include <stdbool.h>
#include "process.h"

#define ANALOGREAD_MUX_BITS 0x1F

void analogread_init(void);

float get_analogvalue(void);
float get_lightvalue(void);

#endif /* ANALOGREAD_H_ */
