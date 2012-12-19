#include "contiki.h"

#include "udp_handler.h"

#include "../../../../../../shared/hexabus_packet.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

PROCESS(hxb_broadcast_handler_process, "Hexabus Broadcast Handler Process");
AUTOSTART_PROCESSES(&hxb_broadcast_handler_process);

PROCESS_THREAD(hxb_broadcast_handler_process, ev, data)
{
  PROCESS_BEGIN();
  PRINTF("Hexabus Broadcast Handler starting.");

  while(1)
  {
    PROCESS_WAIT_EVENT();
   /* 
    if(ev == hxb_broadcast_received_event)
    {
      PRINTF("Broadcast received by hxb_broadcast_handler_process:\r\n");

      // TODO implement at the dotted line

      // . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

      free(data); // careful. It's gone now. If someone else also wants it, we need to think of something more sophisticated.
    }*/
  }

  PROCESS_END();
}

