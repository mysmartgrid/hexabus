#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H 1


#include "../../../../../shared/hexabus_packet.h"

struct hxb_packet_float make_value_packet_float(uint32_t eid, struct hxb_value* val);
struct hxb_packet_int32 make_value_packet_int32(uint32_t eid, struct hxb_value* val);
struct hxb_packet_128string make_epinfo_packet(uint32_t eid);
struct hxb_packet_int8 make_value_packet_int8(uint32_t eid, struct hxb_value* val);
struct hxb_packet_error make_error_packet(uint8_t errorcode);

#endif /* PACKET_BUILDER_H */

