#ifndef LIBHEXABUS_CRC_HPP
#define LIBHEXABUS_CRC_HPP 1

#include <libhexabus/common.hpp>
#include <stdlib.h>

namespace hexabus {
  class CRC {
    public:
      typedef std::tr1::shared_ptr<CRC> Ptr;
      CRC () {};
      uint16_t crc16(const char* input, unsigned int length);
      virtual ~CRC() {};

    private:
      CRC (const CRC& original);
      CRC& operator= (const CRC& rhs);
      
  };
};


#endif /* LIBHEXABUS_CRC_HPP */

