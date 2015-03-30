#ifndef LIBHEXABUS_SERIALIZATION_HPP
#define LIBHEXABUS_SERIALIZATION_HPP 1

#include <vector>

#include <libhexabus/packet.hpp>

namespace hexabus {
	std::vector<char> serialize(const Packet& packet);

	void deserialize(const void* packet, size_t size, PacketVisitor& handler);
	Packet::Ptr deserialize(const void* packet, size_t size);
}

#endif
