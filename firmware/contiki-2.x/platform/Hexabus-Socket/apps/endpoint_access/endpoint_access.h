#ifndef ENDPOINT_ACCESS_H_
#define ENDPOINT_ACCESS_H_

#include <stdint.h>
#include "../../../../../../shared/hexabus_packet.h"

uint8_t get_datatype(uint32_t eid);
uint8_t write(uint32_t eid, struct hxb_value value);
struct hxb_value read(uint32_t eid);

#endif
