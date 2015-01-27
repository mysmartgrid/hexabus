#ifndef LIBHEXABUS_CRC_HPP
#define LIBHEXABUS_CRC_HPP 1

#include <cstddef>
#include <cstdint>

namespace hexabus {
	uint16_t crc(const void* input, size_t size);
};

#endif /* LIBHEXABUS_CRC_HPP */

