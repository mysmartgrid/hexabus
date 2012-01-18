#ifndef ANALOGREAD_H_
#define ANALOGREAD_H_ 

#include <stdbool.h>
#include "process.h"


void analogread_init(void);

float get_analogvalue(void);

#endif /* ANALOGREAD_H_ */
