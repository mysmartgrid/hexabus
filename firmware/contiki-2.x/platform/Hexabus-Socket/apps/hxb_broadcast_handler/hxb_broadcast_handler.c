#include "contiki.h"

#include "udp_handler.h"

#include "../../../../../../shared/hexabus_packet.h"
#include "hxb_switching_rules.h"

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

#define HXB_MAX_RULES 16
struct hxb_switching_rule rules[HXB_MAX_RULES];

PROCESS(hxb_broadcast_handler_process, "Hexabus Broadcast Handler Process");
AUTOSTART_PROCESSES(&hxb_broadcast_handler_process);

PROCESS_THREAD(hxb_broadcast_handler_process, ev, data)
{
  PROCESS_BEGIN();
  PRINTF("Hexabus Broadcast Handler starting.");

  // initialize rules.
  int i;
  for(i = 0; i < HXB_MAX_RULES; i++)
  {
    rules[i].op = HXB_OP_NULL;
  }

  /* char ip[16] = { 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0xc4, 0xff, 0xfe, 0x04, 0x00, 0x0e };
  // make a rule for testing
  memcpy(&rules[4].device, &ip, 16);
  memcpy(&rules[5].device, &ip, 16);
  rules[4].vid = rules[5].vid = 5;
  rules[4].op = HXB_OP_LESSTHAN;
  rules[5].op = HXB_OP_GREATERTHAN;
  rules[4].constant = rules[5].constant = 5;
  rules[4].target_vid = rules[5].target_vid = 0;
  rules[4].target_value = 0;
  rules[5].target_value = 1; */



  while(1)
  {
    PROCESS_WAIT_EVENT();
    
    if(ev == hxb_broadcast_received_event)
    {
      PRINTF("Broadcast received by hxb_broadcast_handler_process:\r\n");
      struct hxb_data_int* value = (struct hxb_data_int*)data;
      PRINTF("Source IP:\t");
      PRINT6ADDR(value->source);
      PRINTF("\ndata type:\t%d\nVID:\t%d\nValue:\t%d\n", value->datatype, value->vid, value->value);

      // look at the switching rules and see if one applies, then execute this
      for(i = 0; i < HXB_MAX_RULES; i++)
      {
        if(rules[i].op)
        {
          if(!memcmp(rules[i].device, value->source, 16)
              && rules[i].vid == value->vid
              // TODO datatype!!
              )
          {
            if( (rules[i].op == HXB_OP_EQUALS && value->value == rules[i].constant)
              || (rules[i].op == HXB_OP_LESSTHAN && value->value < rules[i].constant)
              || (rules[i].op == HXB_OP_GREATERTHAN && value->value > rules[i].constant) )
              // TODO abstraction! we should just call some function setvid(vid, value) here, which takes care of the rest (switching, setting output pins, whatever)
              // for now, just the relay can do something:
              if(rules[i].target_vid == 0)
              {
                if(rules[i].target_value == 0)
                  relay_on();
                else
                  relay_off();
              }
          }
        }
      }

      free(value); // careful. It's gone now. If someone else also wants it, we need to think of something more sophisticated.
    }
  }

  PROCESS_END();
}

