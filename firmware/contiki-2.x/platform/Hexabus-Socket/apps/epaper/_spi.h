#include <stdint.h>
#include <avr/io.h>

static uint8_t spi_rw(uint8_t data) {
  SPDR = data; 
  while (!(SPSR & 0x80)); 
  return SPDR; 
}
