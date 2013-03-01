#ifndef ENDPOINT_ACCESS_H_
#define ENDPOINT_ACCESS_H_

#include <stdint.h>
#include "../../../../../../shared/hexabus_packet.h"

uint8_t endpoint_get_datatype(uint32_t eid);
uint8_t endpoint_write(uint32_t eid, struct hxb_value* value);
void endpoint_read(uint32_t eid, struct hxb_value* val);
void endpoint_get_name(uint32_t eid, char* buffer);

#endif
