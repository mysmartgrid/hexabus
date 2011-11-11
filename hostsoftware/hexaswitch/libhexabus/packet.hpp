#ifndef LIBHEXABUS_PACKET_HPP
#define LIBHEXABUS_PACKET_HPP 1

#include <stdint.h>
#include "../../../shared/hexabus_packet.h"

//#define ENABLE_LOGGING 0
#include <config.h>

/* Include TR1 shared ptrs in a portable way. */
#include <cstddef> // for __GLIBCXX__
#ifdef __GLIBCXX__
#  include <tr1/memory>
#else
#  ifdef __IBMCPP__
#    define __IBMCPP_TR1__
#  endif
#  include <memory>
#endif

#ifdef ENABLE_LOGGING
#include <iostream>
#define LOG(msg) std::cout << msg << std::endl;
#else
#define LOG(msg) 
#endif

namespace hexabus {
  class Packet {
    public:
      typedef std::tr1::shared_ptr<Packet> Ptr;
      Packet() {};
      virtual ~Packet() {};

      // TODO think about whether to return pointers here
      hxb_packet_query query(uint8_t eid);
      hxb_packet_int8 write8(uint8_t eid, uint8_t datatype, uint8_t value, bool broadcast);
      hxb_packet_int32 write32(uint8_t eid, uint8_t datatype, uint32_t value, bool broadcast);
      hxb_packet_datetime writedt(uint8_t eid, uint8_t datatype, datetime value, bool broadcast);
    private:
  };

  class PacketHandling {
    public:
      PacketHandling(char* data);
      ~PacketHandling() {};

      bool getOkay();
      bool getCRCOkay();
      uint8_t getPacketType();
      uint8_t getErrorcode();
      uint8_t getDatatype();
      uint8_t getEID();
      struct hxb_value getValue();
      // TODO think about whether it's better to calculate this on the fly (in the getters) instead of storing it locally...
      // TODO getFlags (once someone starts actually using the flags)
    private:
      bool okay;
      bool crc_okay;
      uint8_t packet_type;
      uint8_t errorcode;
      uint8_t datatype;
      uint8_t eid;
      struct hxb_value value;
  };
};

#endif
