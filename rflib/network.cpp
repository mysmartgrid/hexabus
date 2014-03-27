#include "network.hpp"

#include <stdexcept>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "binary-formatter.hpp"

void get_random(void* target, size_t len)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("open()");

	read(fd, target, len);
	close(fd);
}

Network::Network(const PAN& pan, uint16_t short_addr, uint64_t hwaddr,
	const KeyLookupDescriptor& out_key, uint32_t frame_counter)
	: _pan(pan), _short_addr(short_addr), _hwaddr(hwaddr), _out_key(out_key),
	  _frame_counter(frame_counter)
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
	Key key = Key::indexed(1 << 1, 0, key_bytes, 0);

	Network result(PAN(pan_id, 2, 0), 0xfffe, hwaddr, key.lookup_desc());

	result._keys.push_back(key);
	return result;
}

void Network::add_key(const Key& key)
{
	if (_keys.size() + 1 > MAX_KEYS)
		throw std::runtime_error("key limit reached");

	_keys.push_back(key);
}

void Network::add_device(const Device& dev)
{
	if (_devices.size() + 1 > MAX_DEVICES)
		throw std::runtime_error("device limit reached");

	_devices.push_back(dev);
}



namespace {

void save(BinaryFormatter& fmt, const KeyLookupDescriptor& desc)
{
	fmt.put_u8(desc.mode());
	if (desc.mode() != 0) {
		fmt.put_u8(desc.id());
		switch (desc.mode()) {
		case 2: fmt.put_u32(boost::get<uint32_t>(desc.source())); break;
		case 3: fmt.put_u64(boost::get<uint64_t>(desc.source())); break;
		}
	}
}

KeyLookupDescriptor load_kld(BinaryFormatter& fmt)
{
	uint8_t mode = fmt.get_u8();
	if (mode != 0) {
		uint8_t id = fmt.get_u8();
		switch (mode) {
		case 1: return KeyLookupDescriptor(mode, id);
		case 2: return KeyLookupDescriptor(mode, id, fmt.get_u32());
		case 3: return KeyLookupDescriptor(mode, id, fmt.get_u64());
		}
	}

	return KeyLookupDescriptor(mode);
}



void save(BinaryFormatter& fmt, const Key& key)
{
	save(fmt, key.lookup_desc());
	fmt.put_u8(key.frame_types());
	fmt.put_u32(key.cmd_frame_ids());
	fmt.put_raw(key.key(), 16);
}

Key load_key(BinaryFormatter& fmt)
{
	KeyLookupDescriptor ldesc = load_kld(fmt);
	uint8_t frame_types = fmt.get_u8();
	uint32_t cmd_frame_ids = fmt.get_u32();
	uint8_t key[16];

	fmt.get_raw(key, 16);

	return Key(ldesc, frame_types, cmd_frame_ids, key);
}



void save(BinaryFormatter& fmt, const Device& dev)
{
	fmt.put_u16(dev.pan_id());
	fmt.put_u16(dev.short_addr());
	fmt.put_u64(dev.hwaddr());
	fmt.put_u32(dev.frame_ctr());
	save(fmt, dev.key());
}

Device load_device(BinaryFormatter& fmt)
{
	uint16_t pan_id = fmt.get_u16();
	uint16_t short_addr = fmt.get_u16();
	uint64_t hwaddr = fmt.get_u64();
	uint32_t frame_ctr = fmt.get_u32();
	KeyLookupDescriptor ldesc = load_kld(fmt);

	return Device(pan_id, short_addr, hwaddr, frame_ctr, ldesc);
}

}

void Network::save(Eeprom& target)
{
	if (_keys.size() > MAX_KEYS)
		throw std::runtime_error("too many keys");
	if (_devices.size() > MAX_DEVICES)
		throw std::runtime_error("too many devices");

	std::vector<uint8_t> stream;
	BinaryFormatter fmt(stream);

	fmt.put_u8(VERSION_0);
	fmt.put_u16(_pan.pan_id());
	fmt.put_u8(_pan.page());
	fmt.put_u8(_pan.channel());
	fmt.put_u16(_short_addr);
	fmt.put_u64(_hwaddr);
	fmt.put_u64(_frame_counter);
	::save(fmt, _out_key);

	fmt.put_u8(_keys.size());
	for (size_t i = 0; i < _keys.size(); i++)
		::save(fmt, _keys[i]);

	fmt.put_u8(_devices.size());
	for (size_t i = 0; i < _devices.size(); i++)
		::save(fmt, _devices[i]);

	target.write_stream(stream);
}

Network Network::load(Eeprom& source)
{
	std::vector<uint8_t> stream = *source.read_stream();
	BinaryFormatter fmt(stream);

	if (fmt.get_u8() != VERSION_0)
		throw std::runtime_error("can't load");

	uint16_t pan_id = fmt.get_u16();
	uint16_t page = fmt.get_u8();
	uint16_t channel = fmt.get_u8();

	uint16_t short_addr = fmt.get_u16();
	uint64_t hwaddr = fmt.get_u64();
	uint64_t frame_counter = fmt.get_u64();
	KeyLookupDescriptor out_key = load_kld(fmt);

	Network result(PAN(pan_id, page, channel), short_addr, hwaddr, out_key,
			frame_counter);

	uint8_t keys = fmt.get_u8();
	while (keys-- > 0)
		result._keys.push_back(load_key(fmt));

	uint8_t devices = fmt.get_u8();
	while (devices-- > 0)
		result._devices.push_back(load_device(fmt));

	return result;
}
