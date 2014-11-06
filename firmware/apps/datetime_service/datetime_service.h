#ifndef DATETIME_SERVICE_H_
#define DATETIME_SERVICE_H_

#include <stdbool.h>
#include "process.h"
#include "hexabus_packet.h"

/* skip superfluous seconds if the system clock is fast or insert extra seconds if the clock is slow.
 * set
 *   rate := <actual ticks per second>
 *   irate := <contiki ticks per second>
 *   secPerDay := 86400 * rate / irate
 *   extra := secPerDay - secPerDay
 *   skipEvery := 86400 / extra
 */
#if PLATFORM_TYPE == HEXABUS_SOCKET
/* the clock is defined to have 126 ticks per second, at 8MHz system clock the timer prescaler
 * from clock-avr.h we get 126.008something ticks per second with a timer rate of 8e6 / 1024.
 */
# define DTS_SKIP_EVERY_N 15624
#elif PLATFORM_TYPE == HEXABUS_STM
/* SysTick 32MHz -> 125 ticks is exact
 */
# define DTS_SKIP_EVERY_N 0
#else
# error no skip counters set
#endif

void updateDatetime(struct hxb_envelope* envelope);
int getDatetime(uint64_t* dt);

PROCESS_NAME(datetime_service_process);

#endif /* DATETIME_SERVICE_H_ */
