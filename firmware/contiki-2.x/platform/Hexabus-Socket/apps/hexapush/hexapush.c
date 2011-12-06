#include "hexapush.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "hexabus_config.h"
#include "value_broadcast.h"

#include <stdio.h>

#if HEXAPUSH_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint8_t button_vector = 0;
static uint8_t pressed_vector = 0;
static uint8_t clicked_vector = 0;

void button_clicked(uint8_t button) {

    PRINTF("Clicked %d\n", button);
   
    clicked_vector|=(1<<button);
    broadcast_value(25);
    clicked_vector&=~(1<<button);
}

void button_pressed(uint8_t button) {
    PRINTF("Pressed %d\n", button);
    
    pressed_vector|=(1<<button);

    broadcast_value(24);
}

void button_released(uint8_t button){
    PRINTF("Released %d\n", button);
    
    pressed_vector&=~(1<<button);
   
    broadcast_value(24);
}

uint8_t get_buttonstate() {
    return pressed_vector;
}

uint8_t get_clickstate() {
    return clicked_vector;
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
    HEXAPUSH_DDR &= ~(1<<HEXAPUSH_B1);
    HEXAPUSH_PORT |= button_vector;
}

PROCESS(hexapush_process, "Handles pushbutton presses");

AUTOSTART_PROCESSES(&hexapush_process);

PROCESS_THREAD(hexapush_process, ev, data) {
    
    static struct etimer debounce_timer;

    static uint8_t button_state[8];
    static uint8_t longclick_counter[8];

    PROCESS_BEGIN();

    int i;
    for(i=0;i<8;i++) {
        button_state[i]=0;
        longclick_counter[i]=0;
    }

    hexapush_init();

    etimer_set(&debounce_timer, CLOCK_SECOND * HP_DEBOUNCE_TIME / 1000);
    PRINTF("Hexapush process ready!\n");
   

    while(1) {

         etimer_restart(&debounce_timer);
         int i;

         for(i=0; i<8; i++) {
            if(button_state[i]==0) { //Not pressed state
                //PRINTF("Hexapush: %d is in state 0\n",i);
                if(((~HEXAPUSH_IN&button_vector)&(1<<i))!=0) {
                    button_state[i]=1; //Goto debounce state
                }
            } else if (button_state[i]==1) { //Debounce state
                //PRINTF("Hexapush: %d is in state 1\n",i);
                if(((~HEXAPUSH_IN&button_vector)&(1<<i))!=0) {
                    button_state[i]=2; //Goto click state
                } else {
                    button_state[i]=0;
                }
            } else if (button_state[i]==2) { //Click state
                //PRINTF("Hexapush: %d is in state 2\n",i);
                if(((~HEXAPUSH_IN&button_vector)&(1<<i))!=0) {
                    if(longclick_counter[i]<LONG_CLICK_MULT) {
                        longclick_counter[i]++;
                    } else {
                        button_pressed(i);
                        longclick_counter[i]=0;
                        button_state[i]=3;
                    }
                } else {
                    button_clicked(i);
                    longclick_counter[i]=0;
                    button_state[i]=0;
                }
            } else if (button_state[i]==3) { //Long click state
                //PRINTF("Hexapush: %d is in state 3\n",i);
                if(((~HEXAPUSH_IN&button_vector)&(1<<i))==0) {
                    button_released(i);
                    button_state[i]=0;
                }
            } else {
                PRINTF("Hexapush error: Unknown state\n");
            }
         }
         PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&debounce_timer));
    }

    PROCESS_END();
}

