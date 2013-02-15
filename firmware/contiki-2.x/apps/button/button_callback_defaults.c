#include "contiki.h"

static void button_event_default(void);
static void button_event_default()
{
}

void button_clicked(void) __attribute__((weak, alias("button_event_default")));
void button_long_clicked(uint8_t released) __attribute__((weak, alias("button_event_default")));
void button_held(void) __attribute__((weak, alias("button_event_default")));
