
/*
 * Copyright (c) 2011, Fraunhofer ITWM
 * All rights reserved.
 * Author: 	Mathias Dalheimer <dalheimer@itwm.fhg.de>
 *
 */
#include "memory_debugger.h"
#include "contiki.h"
#include "sys/etimer.h"


PROCESS(memory_debugger_process, "Memory Debugger Process");
AUTOSTART_PROCESSES(&memory_debugger_process);

#define PRINTF(...) printf(__VA_ARGS__)
//static struct etimer memory_debugger_periodic_timer;

/*---------------------------------------------------------------------------*/
static void
pollhandler(void) {
  PRINTF("----Socket_memory_debugger_handler: Process polled\r\n");
}

static void
exithandler(void) {
  PRINTF("----Socket_memory_debugger_handler: Process exits.\r\n");
}
/*---------------------------------------------------------------------------*/

void print_mem_usage(void) {
  PRINTF("MEMDBG: %d bytes free\r\n",
      get_current_free_bytes());
}

void
memory_debugger_init(void)
{
  PRINTF("-- MEMDBG: INIT\r\n");
  print_mem_usage();
}

PROCESS_THREAD(memory_debugger_process, ev, data)
{
  static struct etimer periodic;

  PROCESS_BEGIN();
  PROCESS_PAUSE();
  memory_debugger_init();
  PRINTF("MEMDBG: process started\n");


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
