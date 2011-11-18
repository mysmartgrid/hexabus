#include "hexapush.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"

#include <stdio.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint8_t button_vector = 0x00;

void button_clicked(uint8_t button, uint8_t vector) {
    PRINTF("Clicked %d\n", button);
}

void button_pressed(uint8_t button, uint8_t vector) {
    PRINTF("Pressed %d\n", button);
}

void button_released(uint8_t button, uint8_t vector){
    PRINTF("Released %d\n", button);
}

void hexapush_init(void) {
    PRINTF("Hexapush init\n");


    #if defined(HEXAPUSH_B1)
    button_vector |= (1<<HEXAPUSH_B1);
    #endif
    #if defined(HEXAPUSH_B2)
    button_vector |= (1<<HEXAPUSH_B2);
    #endif
    #if defined(HEXAPUSH_B3)
    button_vector |= (1<<HEXAPUSH_B3);
    #endif
    #if defined(HEXAPUSH_B4)
    button_vector |= (1<<HEXAPUSH_B4);
    #endif
    #if defined(HEXAPUSH_B5)
    button_vector |= (1<<HEXAPUSH_B5);
    #endif
    #if defined(HEXAPUSH_B6)
    button_vector |= (1<<HEXAPUSH_B6);
    #endif
    #if defined(HEXAPUSH_B7)
    button_vector |= (1<<HEXAPUSH_B7);
    #endif
    #if defined(HEXAPUSH_B8)
    button_vector |= (1<<HEXAPUSH_B8);
    #endif

    HEXAPUSH_DDR |= button_vector;
    HEXAPUSH_PORT |= button_vector;
}

PROCESS(hexapush_process, "Handles pushbutton presses");

AUTOSTART_PROCESSES(&hexapush_process);

PROCESS_THREAD(hexapush_process, ev, data) {
    
    static struct etimer debounce_timer;

    static struct etimer* longclick_timers[8];
    
    #if defined(HEXAPUSH_B1)
    static struct etimer b1_longclicktimer;
    #endif
    #if defined(HEXAPUSH_B2)
    static struct etimer b2_longclicktimer;
    #endif
    #if defined(HEXAPUSH_B3)
    static struct etimer b3_longclicktimer;
    #endif
    #if defined(HEXAPUSH_B4)
    static struct etimer b4_longclicktimer;
    #endif
    #if defined(HEXAPUSH_B5)
    static struct etimer b5_longclicktimer;
    #endif
    #if defined(HEXAPUSH_B6)
    static struct etimer b6_longclicktimer;
    #endif   
    #if defined(HEXAPUSH_B7)
    static struct etimer b7_longclicktimer;
    #endif
    #if defined(HEXAPUSH_B8)
    static struct etimer b8_longclicktimer;
    #endif  
 
    static uint8_t pressed_vector;

    ////////////////
    PROCESS_BEGIN();
    ////////////////
    
    #if defined(HEXAPUSH_B1)
    etimer_set(&b1_longclicktimer, CLOCK_SECOND * LONG_CLICK_DELAY / 1000);
    longclick_timers[HEXAPUSH_B1] = &b1_longclicktimer;
    #else
    longclick_timers[HEXAPUSH_B1] = NULL;
    #endif
    #if defined(HEXAPUSH_B2)
    etimer_set(&b2_longclicktimer, CLOCK_SECOND * LONG_CLICK_DELAY / 1000);
    longclick_timers[HEXAPUSH_B2] = &b2_longclicktimer;
    #else
    longclick_timers[HEXAPUSH_B2] = NULL;
    #endif
    #if defined(HEXAPUSH_B3)
    etimer_set(&b3_longclicktimer, CLOCK_SECOND * LONG_CLICK_DELAY / 1000);
    longclick_timers[HEXAPUSH_B3] = &b3_longclicktimer;
    #else
    longclick_timers[HEXAPUSH_B3] = NULL;
    #endif
    #if defined(HEXAPUSH_B4)
    etimer_set(&b4_longclicktimer, CLOCK_SECOND * LONG_CLICK_DELAY / 1000);
    longclick_timers[HEXAPUSH_B4] = &b4_longclicktimer;
    #else
    longclick_timers[HEXAPUSH_B4] = NULL;
    #endif
    #if defined(HEXAPUSH_B5)
    etimer_set(&b5_longclicktimer, CLOCK_SECOND * LONG_CLICK_DELAY / 1000);
    longclick_timers[HEXAPUSH_B5] = &b5_longclicktimer;
    #else
    longclick_timers[HEXAPUSH_B5] = NULL;
    #endif
    #if defined(HEXAPUSH_B6)
    etimer_set(&b6_longclicktimer, CLOCK_SECOND * LONG_CLICK_DELAY / 1000);
    longclick_timers[HEXAPUSH_B6] = &b6_longclicktimer;
    #else
    longclick_timers[HEXAPUSH_B6] = NULL;
    #endif   
    #if defined(HEXAPUSH_B7)
    etimer_set(&b7_longclicktimer, CLOCK_SECOND * LONG_CLICK_DELAY / 1000);
    longclick_timers[HEXAPUSH_B7] = &b7_longclicktimer;
    #else
    longclick_timers[HEXAPUSH_B7] = NULL;
    #endif
    #if defined(HEXAPUSH_B8)
    etimer_set(&b8_longclicktimer, CLOCK_SECOND * LONG_CLICK_DELAY / 1000);
    longclick_timers[HEXAPUSH_B8] = &b8_longclicktimer;
    #else
    longclick_timers[HEXAPUSH_B8] = NULL;
    #endif
    

    hexapush_init();

    etimer_set(&debounce_timer, CLOCK_SECOND * DEBOUNCE_TIME / 1000);
    PRINTF("Hexapush process ready!\n");
   
    pressed_vector = 0;

    while(1) {
         int i=0;

         for(i=0; i<8; i++) {
            if(longclick_timers[i] != NULL) {
                if((~HEXAPUSH_IN&(1<<i))!=0) { //Button pressed?
                    if((pressed_vector&(1<<i))==0) { //Button was not pressed before
                        etimer_stop(longclick_timers[i]);
                        etimer_restart(longclick_timers[i]);
                    } else {
                        if(etimer_expired(longclick_timers[i])) {
                            //TODO Check if already pressed
                            button_pressed(i,pressed_vector);
                        }
                    }
                    pressed_vector|=(1<<i);
                } else {
                    if((pressed_vector&(1<<i))!=0) { //Button was pressed before
                        if(etimer_expired(longclick_timers[i])) {
                            button_released(i,pressed_vector);
                        } else {
                            button_clicked(i,pressed_vector);
                        }
                    }
                    pressed_vector&= ~(1<<i);
                }
            }
         }
         PROCESS_PAUSE();
    }

    PROCESS_END();
}

