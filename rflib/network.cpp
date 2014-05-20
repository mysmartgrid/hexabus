#include "network.hpp"

#include <stdexcept>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/if_alg.h>

#include <boost/foreach.hpp>
#include <boost/format.hpp>

#include "binary-formatter.hpp"
#include "fd.hpp"

void get_random(void* target, size_t len)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("open()");

	read(fd, target, len);
	close(fd);
}

Network::Network(const PAN& pan, uint16_t short_addr, uint64_t hwaddr,
	const KeyLookupDescriptor& out_key, uint32_t frame_counter,
	uint64_t key_id)
	: _pan(pan), _short_addr(short_addr), _hwaddr(hwaddr), _out_key(out_key),
	  _frame_counter(frame_counter), _key_id(key_id)
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

namespace {
struct KeyByDesc {
	KeyLookupDescriptor desc;
	bool operator()(const Key& key)
	{
		return key.lookup_desc() == desc;
	}
};
}

std::vector<Key>::iterator Network::find_key(const KeyLookupDescriptor& desc)
{
	KeyByDesc pred = { desc };
	return std::find_if(_keys.begin(), _keys.end(), pred);
}

void Network::add_key(const Key& key)
{
	if (_keys.size() + 1 > MAX_KEYS)
		throw std::runtime_error("key limit reached");

	if (key.lookup_desc().mode() != 1)
		throw std::runtime_error("invalid key mode");

	if (find_key(key.lookup_desc()) != _keys.end())
		throw std::runtime_error("key exists");

	_keys.push_back(key);
}

void Network::remove_key(const Key& key)
{
	std::vector<Key>::iterator it = find_key(key.lookup_desc());
	if (it == _keys.end())
		throw std::runtime_error("no such key");

	_keys.erase(it);
}

void Network::out_key(const KeyLookupDescriptor& key)
{
	if (find_key(key) == _keys.end())
		throw std::runtime_error("no such key");

	_out_key = key;
}

void Network::add_device(const Device& dev)
{
	if (_devices.size() + 1 > MAX_DEVICES)
		throw std::runtime_error("device limit reached");

	_devices.push_back(dev);
}

namespace {
boost::array<uint8_t, 16> convert_key_id(uint64_t id)
{
	boost::array<uint8_t, 16> result;
	uint64_t id_bytes = htobe64(id);

	memcpy(result.begin() + 8, &id_bytes, 8);
	return result;
}
}

boost::array<uint8_t, 16> Network::derive_key(uint64_t id) const
{
	FD sock(socket(AF_ALG, SOCK_SEQPACKET, 0));

	if (sock < 0)
		throw std::runtime_error("could not open crypto socket");

	struct sockaddr_alg algdesc = {
		PF_ALG,
		"skcipher",
		0,
		0,
		"ecb(aes)"
	};

	if (bind(sock, reinterpret_cast<const sockaddr*>(&algdesc), sizeof(algdesc)) < 0)
		throw std::runtime_error(std::string("bind: ") + strerror(errno));

	FD tfm(accept(sock, NULL, 0));
	if (tfm < 0)
		throw std::runtime_error(std::string("accept: ") + strerror(errno));

	boost::array<uint8_t, 16> buf = convert_key_id(id);

	const uint8_t key[16] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2,
		0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	const int SOL_ALG = 279;

	if (setsockopt(sock, SOL_ALG, ALG_SET_KEY, key, sizeof(key)) < 0)
		throw std::runtime_error(std::string("setsockopt: ") + strerror(errno));

	msghdr msg_out;
	char out_cbuf[CMSG_SPACE(sizeof(int))];
	iovec iov;

	memset(&msg_out, 0, sizeof(msg_out));
	msg_out.msg_control = out_cbuf;
	msg_out.msg_controllen = sizeof(out_cbuf);
	CMSG_FIRSTHDR(&msg_out)->cmsg_len = CMSG_LEN(sizeof(int));
	CMSG_FIRSTHDR(&msg_out)->cmsg_level = SOL_ALG;
	CMSG_FIRSTHDR(&msg_out)->cmsg_type = ALG_SET_OP;
	*reinterpret_cast<int*>(CMSG_DATA(CMSG_FIRSTHDR(&msg_out))) = ALG_OP_ENCRYPT;
	msg_out.msg_controllen = CMSG_FIRSTHDR(&msg_out)->cmsg_len;

	msg_out.msg_iov = &iov;
	msg_out.msg_iovlen = 1;
	iov.iov_base = buf.begin();
	iov.iov_len = buf.size();

	if (sendmsg(tfm, &msg_out, 0) < 0)
		throw std::runtime_error(std::string("sendmsg: ") + strerror(errno));

	if (read(tfm, buf.begin(), buf.size()) < 0)
		throw std::runtime_error(std::string("read: ") + strerror(errno));

	return buf;
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
	fmt.put_u8(dev.key_mode());
}

Device load_device(BinaryFormatter& fmt)
{
	uint16_t pan_id = fmt.get_u16();
	uint16_t short_addr = fmt.get_u16();
	uint64_t hwaddr = fmt.get_u64();
	uint32_t frame_ctr = fmt.get_u32();
	uint8_t key_mode = fmt.get_u8();

	return Device(pan_id, short_addr, hwaddr, frame_ctr, false, key_mode);
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

	fmt.put_u8(VERSION_1);
	fmt.put_u16(_pan.pan_id());
	fmt.put_u8(_pan.page());
	fmt.put_u8(_pan.channel());
	fmt.put_u16(_short_addr);
	fmt.put_u64(_hwaddr);
	fmt.put_u32(_frame_counter);
	fmt.put_u64(_key_id);
	::save(fmt, _out_key);

	fmt.put_u8(_keys.size());
	std::map<KeyLookupDescriptor, uint8_t> key_idx;
	BOOST_FOREACH(const Key& key, _keys) {
		::save(fmt, key);

		key_idx.insert(std::make_pair(key.lookup_desc(), key_idx.size()));
	}

	fmt.put_u8(_devices.size());
	BOOST_FOREACH(const Device& dev, _devices) {
		::save(fmt, dev);

		fmt.put_u8(dev.keys().size());
		typedef std::pair<KeyLookupDescriptor, uint32_t> p_t;
		BOOST_FOREACH(const p_t& devkey, dev.keys()) {
			fmt.put_u8(key_idx[devkey.first]);
			fmt.put_u32(devkey.second);
		}
	}

	target.write_stream(stream);
}

Network Network::load(Eeprom& source)
{
	std::vector<uint8_t> stream = *source.read_stream();
	BinaryFormatter fmt(stream);

	uint8_t version = fmt.get_u8();

	if (version == VERSION_FACTORY_NEW) {
		uint64_t hwaddr = fmt.get_u64();

		return random(hwaddr);
	}

	if (version == VERSION_1) {
		uint16_t pan_id = fmt.get_u16();
		uint16_t page = fmt.get_u8();
		uint16_t channel = fmt.get_u8();

		uint16_t short_addr = fmt.get_u16();
		uint64_t hwaddr = fmt.get_u64();
		uint64_t frame_counter = fmt.get_u32();
		uint64_t key_id = fmt.get_u64();
		KeyLookupDescriptor out_key = load_kld(fmt);

		Network result(PAN(pan_id, page, channel), short_addr,
				hwaddr, out_key, frame_counter, key_id);

		uint8_t keys = fmt.get_u8();
		while (keys-- > 0)
			result._keys.push_back(load_key(fmt));

		uint8_t devices = fmt.get_u8();
		while (devices-- > 0) {
			Device dev(load_device(fmt));

			uint8_t devkeys = fmt.get_u8();
			while (devkeys-- > 0) {
				uint8_t idx = fmt.get_u8();
				uint32_t ctr = fmt.get_u32();

				dev.keys().insert(
					std::make_pair(
						result._keys[idx].lookup_desc(),
						ctr));
			}

			result._devices.push_back(dev);
		}

		return result;
	}

	throw std::runtime_error("can't load");
}

void Network::init_eeprom(Eeprom& target, uint64_t addr)
{
	std::vector<uint8_t> stream;
	BinaryFormatter fmt(stream);

	fmt.put_u8(VERSION_FACTORY_NEW);
	fmt.put_u64(addr);

	target.write_stream(stream);
}
