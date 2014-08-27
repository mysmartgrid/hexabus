#ifndef HEXABUS__PLATFORM__CONFIG_H_F09C378DE675F16F
#define HEXABUS__PLATFORM__CONFIG_H_F09C378DE675F16F

typedef uint16_t button_pin_t;

#include <stm32l1xx.h>
#include <string.h>
#include <stdio.h>

#define RODATA
#define ROSTR(x)                  (x)
#define ROSTR_FMT                 "s"
#define MAX_BUTTON_BIT            (0x8000)
#define memcpy_from_rodata(...)   memcpy(__VA_ARGS__)
#define strncpy_from_rodata(...)  strncpy(__VA_ARGS__)
#define printf_rofmt(...)         printf(__VA_ARGS__)

#define BUTTON_PIN          GPIOB->IDR
#define BUTTON_BIT          11
#define BUTTON_ACTIVE_LEVEL 0

#endif
