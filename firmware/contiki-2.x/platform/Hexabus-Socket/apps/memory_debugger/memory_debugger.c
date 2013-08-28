
/*
 * Copyright (c) 2011, Fraunhofer ITWM
 * All rights reserved.
 * Author: 	Mathias Dalheimer <dalheimer@itwm.fhg.de>
 *
 */
#include "memory_debugger.h"
#include "contiki.h"
#include "sys/etimer.h"
#include <stdio.h>


PROCESS(memory_debugger_process, "Memory Debugger Process");
AUTOSTART_PROCESSES(&memory_debugger_process);

#define LOG_LEVEL LOG_DEBUG
#include "syslog.h"

#define PRINTF(...) printf(__VA_ARGS__)
//static struct etimer memory_debugger_periodic_timer;

/*---------------------------------------------------------------------------*/
static void
pollhandler(void) {
	syslog(LOG_DEBUG, "Process polled");
}

static void
exithandler(void) {
	syslog(LOG_DEBUG, "Process exits.");
}
/*---------------------------------------------------------------------------*/

void print_mem_usage(void) {
	syslog(LOG_DEBUG, "%d bytes free", get_current_free_bytes());
}

void
memory_debugger_init(void)
{
	syslog(LOG_DEBUG, "INIT");
	print_mem_usage();
}

PROCESS_THREAD(memory_debugger_process, ev, data)
{
  static struct etimer periodic;

  PROCESS_BEGIN();
  PROCESS_PAUSE();
  memory_debugger_init();
  syslog(LOG_DEBUG, "process started");


  etimer_set(&periodic, MEMORY_DEBUGGER_INTERVAL*CLOCK_SECOND);
  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      print_mem_usage();
    }
  }

  PROCESS_END();
}
