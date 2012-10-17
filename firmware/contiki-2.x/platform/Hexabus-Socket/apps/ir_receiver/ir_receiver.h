#ifndef IR_RECEIVER_H_
#define IR_RECEIVER_H_ 

#include <stdbool.h>
#include "process.h"

#define COMP_VAL 100
#define IR_REP_DELAY 0.3
#define IR_SAMSUNG 1   // 0 for standard NEC protocol

                       //IR31.........................IR0
#define IR_REPEAT_MASK 0b00010100001111000111100000000000

//Code mapping (Samsung BN59-00940A remote)
#define IR0 3994093319 //0
#define IR1 4211345159 //1
#define IR2 4194633479 //2
#define IR3 4177921799 //3
#define IR4 4144498439 //4
#define IR5 4127786759 //5
#define IR6 4111075079 //6
#define IR7 4077651719 //7
#define IR8 4060940039 //8
#define IR9 4044228359 //9
#define IR10 4244768519 //Power
#define IR11 4161210119 //Vol+
#define IR12 4094363399 //Vol-
#define IR13 3977381639 //Chan+
#define IR14 4010804999 //Chan-
#define IR15 4027516679 //Mute
#define IR16 3843688199 //Menu
#define IR17 4261480199 //Source
#define IR18 2673870599 //Up
#define IR19 2657158919 //Down
#define IR20 2590312199 //Left
#define IR21 2640447239 //Right
#define IR22 2540177159 //Enter
#define IR23 2807564039 //Return
#define IR24 3526166279 //Exit
#define IR25 3760129799 //Info
#define IR26 3125085959 //Rewind
#define IR27 3041527559 //Pause
#define IR28 3074950919 //Fastforward
#define IR29 3058239239 //Rec
#define IR30 3091662599 //Play
#define IR31 3108374279 //Stop

//State machine
#define IR_IDLE_STATE 0
#define IR_START_STATE 1
#define IR_DATA_STATE 2

#define IR_EDGE_DOWN 0
#define IR_EDGE_UP 1

void ir_receiver_init();
uint32_t ir_get_last_command();

PROCESS_NAME(ir_receiver_process);

#endif /* IR_RECEIVER_H_ */
