#ifndef HEXAPUSH_H_
#define HEXAPUSH_H_

#include <stdint.h>


#define HEXAPUSH_PORT   PORTA
#define HEXAPUSH_PIN    PINA
#define HEXAPUSH_DDR    DDRA

// Set unneeded buttons to 0
#define HEXAPUSH_B0     1
#define HEXAPUSH_B1     1
#define HEXAPUSH_B2     0
#define HEXAPUSH_B3     0
#define HEXAPUSH_B4     0
#define HEXAPUSH_B5     0
#define HEXAPUSH_B6     0
#define HEXAPUSH_B7     0

void    hexapush_init(void);

#endif /* HEXAPUSH_H_ */
