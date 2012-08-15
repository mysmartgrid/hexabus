#include "mem-debug.h"
#include <avr/io.h>
// Mask to init SRAM and check against
#define MASK 0xaa
// declared in the linker script
extern unsigned char __heap_start;
extern unsigned char __heap_end;
extern unsigned char __malloc_margin;
extern void* __brkval;


uint16_t get_current_free_bytes(void) {
  return (SP - (uint16_t) &__brkval);
}


