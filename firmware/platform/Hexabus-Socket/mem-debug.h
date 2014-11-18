#ifndef MEM_DEBUG_H
#define MEM_DEBUG_H

#include <stdint.h>
/*
 * See http://www.rn-wissen.de/index.php/Speicherverbrauch_bestimmen_mit_avr-gcc
 */

/* Memory debugging. This is useful to detect memory leaks since you can 
 * print the current memory usage during runtime. The avr-size tool only
 * reports stack sizes, but cannot calculate heap size. These tools
 * calculate the free memory dynamically during runtime.
 */
uint16_t get_current_free_bytes(void);

#endif /* MEM-DEBUG_H */

