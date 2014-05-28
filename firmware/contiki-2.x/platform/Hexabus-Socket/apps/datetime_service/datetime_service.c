#include "datetime_service.h"
#include <util/delay.h>
#include "sys/clock.h"
#include "sys/etimer.h" //contiki event timer library
#include "contiki.h"

#include "hexabus_config.h"
#include "eeprom_variables.h"
#include <avr/eeprom.h>

#include <stdlib.h>

#define LOG_LEVEL DATETIME_SERVICE_DEBUG
#include "syslog.h"

static struct hxb_datetime current_dt;
static bool time_valid;
static uint32_t timestamp; // seconds since datetime-service was started.

static int valid_counter;
static struct etimer update_timer;

void updateDatetime(struct hxb_datetime dt) {
	syslog(LOG_DEBUG, "Time: Got update.");

	current_dt = dt;
	time_valid = true;
	valid_counter = 0;

	etimer_restart(&update_timer);
}

int getDatetime(struct hxb_datetime *dt) {
    dt->hour = current_dt.hour;
    dt->minute = current_dt.minute;
    dt->second = current_dt.second;
    dt->day = current_dt.day;
    dt->month = current_dt.month;
    dt->year = current_dt.year;
    dt->weekday = current_dt.weekday;

    if(time_valid) {
        return 0;
    } else {
        return -1;
    }
}

uint32_t getTimestamp() {
    return timestamp;
}

PROCESS(datetime_service_process, "Keeps the Date and Time up-to-date\n");

PROCESS_THREAD(datetime_service_process, ev, data) {


    PROCESS_BEGIN();

    timestamp = 0;
    time_valid = false;
    valid_counter = VALID_TIME;

    etimer_set(&update_timer, CLOCK_SECOND * UPDATE_INTERVALL);

    while(1) {
        PROCESS_WAIT_EVENT();

        if(ev == PROCESS_EVENT_TIMER) {
            syslog(LOG_INFO, "Time: %u:%u:%u\t%u.%u.%u Day: %u Valid: %u", current_dt.hour, current_dt.minute, current_dt.second, current_dt.day, current_dt.month, current_dt.year, current_dt.weekday, time_valid);

            if(etimer_expired(&update_timer)){
                etimer_reset(&update_timer);
            }

            timestamp++; // update timestamp

            if(valid_counter < VALID_TIME) {
                valid_counter+=1;
            } else {
                time_valid = false;
            }

            current_dt.second+=1;
            if(current_dt.second > 59) {
                current_dt.second = 0;
                current_dt.minute+=1;
                if(current_dt.minute > 59) {
                    current_dt.minute = 0;
                    current_dt.hour+=1;
                    if(current_dt.hour > 23) {
                        current_dt.hour = 0;
                        current_dt.day+=1;
                        current_dt.weekday+=1;
                        if(current_dt.weekday>6) {
                            current_dt.weekday=0;
                        }
                        if( ((current_dt.month==1||current_dt.month==3||current_dt.month==5||current_dt.month==7||current_dt.month==8||current_dt.month==10||current_dt.month==12)&&current_dt.day>31) || ((current_dt.month==4||current_dt.month==6||current_dt.month==9||current_dt.month==11)&&current_dt.day>30) || (current_dt.month==2 && current_dt.day>28)) { //TODO Schaltjahre >.<
                            current_dt.day=0;
                            current_dt.month+=1;
                            if(current_dt.month>12) {
                                current_dt.month=0;
                                current_dt.year+=1;
                            }
                        }
                    }
                }
            }
        }
    }
    PROCESS_END();
}

