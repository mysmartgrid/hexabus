#include "network.hpp"

#include <stdexcept>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void get_random(void* target, size_t len)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("open()");

	read(fd, target, len);
	close(fd);
}

Network::Network(const PAN& pan, uint16_t short_addr, uint64_t hwaddr,
	const KeyLookupDescriptor& out_key)
	: _pan(pan), _short_addr(short_addr), _hwaddr(hwaddr), _out_key(out_key)
{
}

Network Network::random(uint64_t hwaddr)
{
	if (hwaddr == 0xFFFFFFFFFFFFFFFFULL) {
		uint8_t addr[sizeof(hwaddr)];

		get_random(addr, sizeof(addr));
		addr[0] &= ~1; // not multicast
		addr[0] |= 2; // local
		memcpy(&hwaddr, addr, sizeof(hwaddr));
	}

	uint16_t pan_id;
	get_random(&pan_id, sizeof(pan_id));

	uint8_t key_bytes[16];

	get_random(key_bytes, 16);
	Key key = Key::indexed("", 1 << 1, 0, key_bytes, 0);

	Network result(PAN(pan_id, 2, 0), 0xfffe, hwaddr, key.lookup_desc());

	result._keys.push_back(key);
	return result;
}
