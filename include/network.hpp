#ifndef NETWORK_HPP_
#define NETWORK_HPP_

#include <vector>
#include <map>

#include <stdint.h>

#include "types.hpp"

class Network {
	private:
		PAN _pan;
		uint16_t _short_addr;
		uint64_t _hwaddr;
		std::vector<Key> _keys;
		std::map<Device, Key> _devices;
		KeyLookupDescriptor _out_key;

		Network(const PAN& pan, uint16_t short_addr, uint64_t hwaddr,
			const KeyLookupDescriptor& out_key);
	public:
		static Network random(uint64_t hwaddr = 0xFFFFFFFFFFFFFFFFULL);

		PAN pan() const { return _pan; }
		uint16_t short_addr() const { return _short_addr; }
		uint64_t hwaddr() const { return _hwaddr; }
		const std::vector<Key>& keys() const { return _keys; }
		const std::map<Device, Key>& devices() const { return _devices; }
		const KeyLookupDescriptor& out_key() const { return _out_key; }
};

#endif
