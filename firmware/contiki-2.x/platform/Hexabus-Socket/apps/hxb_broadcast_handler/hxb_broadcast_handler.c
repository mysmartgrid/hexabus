#include "contiki.h"

#include "udp_handler.h"

#include "../../../../../../shared/hexabus_packet.h"
// TODO debug-f00 with PRINTF, not printf.

PROCESS(hxb_broadcast_handler_process, "Hexabus Broadcast Handler Process");
AUTOSTART_PROCESSES(&hxb_broadcast_handler_process);

PROCESS_THREAD(hxb_broadcast_handler_process, ev, data)
{
  PROCESS_BEGIN();
  printf("Hexabus Broadcast Handler starting.");

  while(1)
  {
    PROCESS_WAIT_EVENT();
    
    if(ev == hxb_broadcast_received_event)
    {
      printf("Broadcast received by hxb_broadcast_handler_process:\r\n");
      struct hxb_packet_int* packet = (struct hexabus_packet_int*)data;
      printf("Type:\t%d\nFlags:\t%d\nVID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet->type, packet->flags, packet->vid, packet->datatype, packet->value, packet->crc);
      printf("[[%d]]", packet);
      free(packet); // careful. It's gone now. If someone else also wants it, we need to think of something more sophisticated.
    }
  }

  PROCESS_END();
}

