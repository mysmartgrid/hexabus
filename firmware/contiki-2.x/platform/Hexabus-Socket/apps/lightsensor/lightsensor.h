#ifndef LIGHTSENSOR_H_
#define LIGHTSENSOR_H_

#include <stdbool.h>
#include "process.h"

#define LIGHTSENSOR_PIN 0 // 0 to 7

void lightsensor_init(void);

uint32_t get_lightvalue(void);

#endif /* LIGHTSENSOR_H_ */
