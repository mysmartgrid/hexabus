#ifndef LIBHEXABUS_PACKET_HPP
#define LIBHEXABUS_PACKET_HPP 1

#include <stdint.h>
#include "../../../shared/hexabus_packet.h"

//#define ENABLE_LOGGING 0
#include <config.h>

#ifdef ENABLE_LOGGING
#include <iostream>
#define LOG(msg) std::cout << msg << std::endl;
#else
#define LOG(msg) 
#endif

namespace hexabus {
  class Packet {
    public:
      Packet() {};
      virtual ~Packet() {};

      // TODO think about whether to return pointers here
      hxb_packet_query query(uint8_t eid);
      hxb_packet_int8 setvalue8(uint8_t eid, uint8_t datatype, uint8_t value, bool broadcast);
      hxb_packet_int32 setvalue32(uint8_t eid, uint8_t datatype, uint32_t value, bool broadcast);
    private:
  };
};

#endif
