#ifndef PACKET_BUILDER_H
#define PACKET_BUILDER_H 1


#include "../../../../../shared/hexabus_packet.h"

void packet_finalize_u8(struct hxb_packet_u8* packet);
void packet_finalize_u32(struct hxb_packet_u32* packet);
void packet_finalize_datetime(struct hxb_packet_datetime* packet);
void packet_finalize_float(struct hxb_packet_float* packet);
void packet_finalize_128string(struct hxb_packet_128string* packet);
void packet_finalize_66bytes(struct hxb_packet_66bytes* packet);
void packet_finalize_16bytes(struct hxb_packet_16bytes* packet);

struct hxb_packet_error make_error_packet(enum hxb_error_code code);

#endif /* PACKET_BUILDER_H */

