#include "crc.hpp"
#include <boost/crc.hpp>

namespace hexabus {

uint16_t crc(const void* input, size_t size)
{
	boost::crc_optimal<16, 0x1021, 0x0000, 0, true, true> crc; 
	crc.process_bytes(input, size);
	return crc.checksum();
}

}
