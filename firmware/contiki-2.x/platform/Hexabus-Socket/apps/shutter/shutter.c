#include "hexabus_config.h"

#include "shutter.h"
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"

#include <avr/interrupt.h>

#if SHUTTER_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static int8_t shutter_direction;
static int shutter_goal;
static int shutter_pos;
static int shutter_upperbound;

static process_event_t shutter_poke;

void shutter_init(void) {
    PRINTF("Shutter init\n");
    SHUTTER_DDR |= ( 0x00 | (1<<SHUTTER_OUT_UP) | (1<<SHUTTER_OUT_DOWN) );
    SHUTTER_DDR &= ~( (1<<PA4) | (1<<PA5) );
    SHUTTER_PORT |= ( (1<<PA4) | (1<<PA5) ); //pull-ups for encoder inputs

    /* Enable pin interrupts for encoder ports */
    PCMSK0 |= ( 1<<SHUTTER_ENC1 ); 
    PCMSK0 |= ( 1<<SHUTTER_ENC2 );
    PCICR |= ( 1<<PCIE0 );
    sei();

    shutter_poke = process_alloc_event();

    shutter_goal = 0;
    shutter_direction = SHUTTER_DIR_STOP;
    shutter_pos = SHUTTER_MAX_BOUND/2;
    shutter_upperbound = SHUTTER_MAX_BOUND;

}

void shutter_open(void) {
    PRINTF("Shutter opening\n");
    shutter_direction = SHUTTER_DIR_UP;
    SHUTTER_PORT &= ~(1<<SHUTTER_OUT_DOWN);
    SHUTTER_PORT |= (1<<SHUTTER_OUT_UP);
    process_post(&shutter_process, shutter_poke, NULL);
}

void shutter_close(void) {
    PRINTF("Shutter closing\n");
    shutter_direction = SHUTTER_DIR_DOWN;
    SHUTTER_PORT &= ~(1<<SHUTTER_OUT_UP);
    SHUTTER_PORT |= (1<<SHUTTER_OUT_DOWN);
    process_post(&shutter_process, shutter_poke, NULL);
}

void shutter_stop(void) {
    PRINTF("Shutter stop\n");
    shutter_direction = SHUTTER_DIR_STOP;
    SHUTTER_PORT &= ~( (1<<SHUTTER_OUT_UP) | (1<<SHUTTER_OUT_DOWN) );
}

void shutter_set(uint8_t val) {

    if(val == 255) {
        shutter_goal = SHUTTER_MAX_BOUND; //completely open
    } else if(val == 1) {
        shutter_goal = -SHUTTER_MAX_BOUND; //completely closed
    } else {
        shutter_goal = val * ((float)shutter_upperbound/255.0); //move to given position
    }

    if(shutter_goal > shutter_pos) {
        PRINTF("Shutter moving to %d\n", shutter_goal);
        shutter_open();
    } else if(shutter_goal < shutter_pos){
        PRINTF("Shutter moving to %d\n", shutter_goal);
        shutter_close();
    } else {
        PRINTF("Shutter not moving, already at %d\n", shutter_goal);
    }
}

void shutter_toggle(uint8_t val) {
    if(shutter_direction==SHUTTER_DIR_STOP && val!=0){
        shutter_set(val);
    } else {
        shutter_stop();
    }
}

uint8_t shutter_get_state(void) {
    return shutter_pos * ((float)shutter_upperbound/255.0);
}

PROCESS(shutter_process, "Open/Close shutter until motor stopps");
PROCESS(shutter_setup_process, "Initial calibration and position");

PROCESS_THREAD(shutter_setup_process, ev, data) {

    PROCESS_BEGIN();


    if(SHUTTER_CALIBRATE_ON_BOOT) {
        PRINTF("Shutter calibrating...\n");
        shutter_toggle(1);
        while(shutter_direction != SHUTTER_DIR_STOP) {
            PROCESS_PAUSE();
        }

        shutter_toggle(255);
        while(shutter_direction != SHUTTER_DIR_STOP) {
            PROCESS_PAUSE();
        }
        PRINTF("Shutter calibration finished\n");

    }
    if(SHUTTER_INITIAL_POSITON) {
        PRINTF("Shutter moving to initial position");
        shutter_toggle(SHUTTER_INITIAL_POSITON);
    }

    PROCESS_END();


}

PROCESS_THREAD(shutter_process, ev, data) {

    static struct etimer encoder_timer;

    PROCESS_BEGIN();

    etimer_set(&encoder_timer, CLOCK_SECOND * ENCODER_TIMEOUT / 1000);

    while(1) {
        PROCESS_WAIT_EVENT();

        if(ev == PROCESS_EVENT_POLL) {
            etimer_restart(&encoder_timer);
            shutter_pos += shutter_direction;
            PRINTF("Shutter at position: %d\n", shutter_pos);
            if (((shutter_pos >= shutter_goal)&&(shutter_direction == SHUTTER_DIR_UP)) || ((shutter_pos <= shutter_goal)&&(shutter_direction == SHUTTER_DIR_DOWN))) {
                shutter_stop();
            }
        } else if(ev == PROCESS_EVENT_TIMER) {
            PRINTF("Shutter encoder timed out\n");
            if(shutter_direction == SHUTTER_DIR_UP) {
                shutter_upperbound = shutter_pos;
                PRINTF("Shutter has new upperbound: %d\n", shutter_upperbound);
            } else if(shutter_direction == SHUTTER_DIR_DOWN) {
                shutter_pos = 1;
                PRINTF("Shutter reset position to 1\n");
            }
            shutter_stop();
        } else if(ev == shutter_poke) {
            etimer_restart(&encoder_timer);
        }
    }
    PROCESS_END();
}

ISR(PCINT0_vect) {
    process_poll(&shutter_process);
}
