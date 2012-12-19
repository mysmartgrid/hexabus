#include "crc.hpp"
#include <boost/crc.hpp>

using namespace hexabus;

uint16_t CRC::crc16(const char* input, unsigned int length) {
  boost::crc_optimal<16, 0x1021, 0x0000, 0, true, true> crc; 
  crc.process_bytes(input, length);
  return crc.checksum();
}
