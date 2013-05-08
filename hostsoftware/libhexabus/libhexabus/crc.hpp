#ifndef LIBHEXABUS_CRC_HPP
#define LIBHEXABUS_CRC_HPP 1

#include <libhexabus/common.hpp>
#include <stdlib.h>

namespace hexabus {
	uint16_t crc(const void* input, size_t size);
};

#endif /* LIBHEXABUS_CRC_HPP */

