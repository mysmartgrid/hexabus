#ifndef LIBHEXABUS_SERIALIZATION_HPP
#define LIBHEXABUS_SERIALIZATION_HPP 1

#include <vector>

#include <libhexabus/config.h>
#include <libhexabus/packet.hpp>

namespace hexabus {
	std::vector<char> serialize(const Packet& packet, uint16_t seqNum);

	void deserialize(const void* packet, size_t size, PacketVisitor& handler);
	Packet::Ptr deserialize(const void* packet, size_t size);
}

#endif
