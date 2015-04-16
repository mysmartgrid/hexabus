#ifndef HEXABUS__PLATFORM__CONFIG_H_31767F680F487364
#define HEXABUS__PLATFORM__CONFIG_H_31767F680F487364

typedef uint8_t button_pin_t;

#include <avr/pgmspace.h>
#include <avr/io.h>
#include <stdio.h>

#define RODATA                    PROGMEM
#define ROSTR(x)                  PSTR(x)
#define ROSTR_FMT                 "S"
#define MAX_BUTTON_BIT            (0x80)
#define memcpy_from_rodata(...)   memcpy_P(__VA_ARGS__)
#define strncpy_from_rodata(...)  strncpy_P(__VA_ARGS__)
#define printf_rofmt(...)         printf_P(__VA_ARGS__)

#define BUTTON_PIN          PIND
#define BUTTON_BIT          PD5
#define BUTTON_ACTIVE_LEVEL 0

#endif
