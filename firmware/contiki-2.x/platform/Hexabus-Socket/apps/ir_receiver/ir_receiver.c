#include "ir_receiver.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"
#include "hexabus_config.h"
#include "value_broadcast.h"
#include "hexonoff.h"

#if IR_RECEIVER_DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static uint32_t ir_time = 0;
static uint32_t ir_time_since_last = 0;
static uint8_t ir_data[4] = {0,0,0,0};
static uint8_t ir_state = IR_IDLE_STATE;
static uint8_t ir_edge_dir = IR_EDGE_DOWN;
static uint8_t ir_repeat = 0;
static uint8_t ir_bit = 0;
static uint8_t ir_byte = 0;
static uint8_t ir_last_data[4];

void ir_receiver_init() {

    PRINTF("IR receiver init\n");
    EICRA |= (1<<ISC21 );
    EIMSK |= (1<<INT2 );

    TCCR0A |= (1<<WGM01);
    TIMSK0 |= (1<<OCIE0A);
    OCR0A=COMP_VAL;
    TCCR0B |= ((1<<CS01));

    process_start(&ir_receiver_process, NULL);
    sei();
}

void ir_receiver_reset() {
    ir_data[0] = 0;
    ir_data[1] = 0;
    ir_data[2] = 0;
    ir_data[3] = 0;
    ir_state = IR_IDLE_STATE;
    EICRA &= ~((1<<ISC21)|(1<<ISC20));
    EICRA |= (1<<ISC21);
    ir_edge_dir = IR_EDGE_DOWN;
}

uint32_t ir_get_last_command() {
#if IR_RECEIVER_RAW_MODE
    return *(uint32_t*) ir_last_data;
#else
    switch(*(uint32_t*) ir_last_data) {
        case IR0:
            return 0x1;
            break;
        case IR1:
            return 0x2;
            break;
        case IR2:
            return 0x4;
            break;
        case IR3:
            return 0x8;
            break;
        case IR4:
            return 0x10;
            break;
        case IR5:
            return 0x20;
            break;
        case IR6:
            return 0x40;
            break;
        case IR7:
            return 0x80;
            break;
        case IR8:
            return 0x100;
            break;
        case IR9:
            return 0x200;
            break;
        case IR10:
            return 0x400;
            break;
        case IR11:
            return 0x800;
            break;
        case IR12:
            return 0x1000;
            break;
        case IR13:
            return 0x2000;
            break;
        case IR14:
            return 0x4000;
            break;
        case IR15:
            return 0x8000;
            break;
        case IR16:
            return 0x10000;
            break;
        case IR17:
            return 0x20000;
            break;
        case IR18:
            return 0x40000;
            break;
        case IR19:
            return 0x80000;
            break;
        case IR20:
            return 0x100000;
            break;
        case IR21:
            return 0x200000;
            break;
        case IR22:
            return 0x400000;
            break;
        case IR23:
            return 0x800000;
            break;
        case IR24:
            return 0x1000000;
            break;
        case IR25:
            return 0x2000000;
            break;
        case IR26:
            return 0x4000000;
            break;
        case IR27:
            return 0x8000000;
            break;
        case IR28:
            return 0x10000000;
            break;
        case IR29:
            return 0x20000000;
            break;
        case IR30:
            return 0x40000000;
            break;
        case IR31:
            return 0x80000000;
            break;
        default:
            return 0;
    }
#endif
}

PROCESS(ir_receiver_process, "Listen for IR commands");

PROCESS_THREAD(ir_receiver_process, ev, data) {

    PROCESS_BEGIN();

    while(1) {
        PROCESS_WAIT_EVENT();

        if(ev == PROCESS_EVENT_POLL) {
            if(ir_repeat) {
                PRINTF("Got repeat signal %d \n", ir_time_since_last);
                ir_repeat = 0;
            } else {
                PRINTF("Got new command %d,%d,%d,%d!\n", ir_last_data[0],ir_last_data[1],ir_last_data[2],ir_last_data[3]);
            }
            broadcast_value(30);
        }
    }

    PROCESS_END();
}

ISR(INT2_vect) {
    EIMSK &= ~(1<<INT2);

    ir_time_since_last = ir_time;
    ir_time = 0;
    TCNT0 = 0;

    switch(ir_state) {
        case IR_IDLE_STATE:                  //Waiting for AGC burst
            if(!ir_edge_dir) {   // Beginning of possible burst
                EICRA |= ((1<<ISC21)|(1<<ISC20));
                ir_edge_dir = IR_EDGE_UP;
            } else {             // Possible end of burst
#if IR_SAMSUNG
                if( (ir_time_since_last>40)&&(ir_time_since_last<47) ) {
#else
                if( (ir_time_since_last>85)&&(ir_time_since_last<95) ) {
#endif
                    ir_state = IR_START_STATE;
                    EICRA &= ~((1<<ISC21)|(1<<ISC20));
                    EICRA |= (1<<ISC21);
                    ir_edge_dir = IR_EDGE_DOWN;
    EIMSK &= ~(1<<INT2);
                } else {
                    ir_receiver_reset();
                }
            }
            break;

        case IR_START_STATE:                  //Waiting for AFC gap
            if( (ir_time_since_last>40)&&(ir_time_since_last<47) ) {

                ir_state = IR_DATA_STATE;
                ir_bit = 0;
                ir_byte = 0;

                EICRA |= ((1<<ISC21)|(1<<ISC20));
                ir_edge_dir = IR_EDGE_UP;

            } else if( (ir_time_since_last>20)&&(ir_time_since_last<25) ) {
                ir_repeat = 1;
                process_poll(&ir_receiver_process);
                ir_receiver_reset();
            } else {
                ir_receiver_reset();
            }
            break;
        case IR_DATA_STATE:              //Waiting for bits
            if(ir_edge_dir) {
                if( ir_byte > 3) {
                    memcpy(ir_last_data, ir_data, 4);
                    process_poll(&ir_receiver_process);
                    ir_receiver_reset();
                } else if( (ir_time_since_last>1)&&(ir_time_since_last<7) ) {
                    EICRA &= ~((1<<ISC21)|(1<<ISC20));
                    EICRA |= (1<<ISC21);
                    ir_edge_dir = IR_EDGE_DOWN;
                } else {
                    ir_receiver_reset();
                }
            } else {
                if( (ir_time_since_last>1)&&(ir_time_since_last<7) ) {
                    ir_bit++;
                    if(ir_bit > 7) {
                        ir_bit = 0;
                        ir_byte++;
                    }
                    EICRA |= ((1<<ISC21)|(1<<ISC20));
                    ir_edge_dir = IR_EDGE_UP;
                } else if( (ir_time_since_last>14)&&(ir_time_since_last<19)) {
                    ir_data[ir_byte] |= (1<<ir_bit);
                    ir_bit++;
                    if(ir_bit > 7) {
                        ir_bit = 0;
                        ir_byte++;
                    }
                    EICRA |= ((1<<ISC21)|(1<<ISC20));
                    ir_edge_dir = IR_EDGE_UP;
                } else {
                    ir_receiver_reset();
                }
            }
            break;

        default:
            break;
    }

    EIMSK |= (1<<INT2 );
}

ISR(TIMER0_COMPA_vect) {
    ir_time++;
}

