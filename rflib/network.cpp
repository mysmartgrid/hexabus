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

namespace {

void get_random(void* target, size_t len)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("open()");

	read(fd, target, len);
	close(fd);
}

KeyLookupDescriptor kld(uint8_t id)
{
	return KeyLookupDescriptor(1, id);
}

}



Network::Network(const PAN& pan, uint16_t short_addr, uint64_t hwaddr,
	const SecurityParameters& sec_params,
	const boost::array<uint8_t, 16>& master_key)
	: _pan(pan), _short_addr(short_addr), _hwaddr(hwaddr),
	  _sec_params(sec_params), _master_key(master_key), _key_epoch(0)
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

	boost::array<uint8_t, 16> key_bytes;
	get_random(key_bytes.c_array(), 16);

	Network result(PAN(pan_id, DEFAULT_PAGE, DEFAULT_CHANNEL), 0xfffe,
			hwaddr,
			SecurityParameters(true, DEFAULT_SECLEVEL, kld(0), 0),
			key_bytes);

	result.add_keys_until(1);

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

void Network::sec_params(const SecurityParameters& params)
{
	if (find_key(params.out_key()) == _keys.end())
		throw std::runtime_error("invalid out_key");

	_sec_params = params;
}

void Network::add_device(const Device& dev)
{
	if (_devices.size() + 1 > MAX_DEVICES)
		throw std::runtime_error("device limit reached");

	_devices.push_back(dev);
}

void Network::remove_device(const Device& dev)
{
	std::vector<Device>::iterator it = std::find(_devices.begin(), _devices.end(), dev);
	if (it == _devices.end())
		throw std::runtime_error("no such device");

	_devices.erase(it);
}

void Network::advance_key_epoch(uint64_t to_epoch)
{
	if (to_epoch - _key_epoch >= 255)
		throw std::runtime_error("epoch step too large");

	uint8_t skipped = to_epoch - _key_epoch;
	uint8_t epoch_start = _key_epoch & 0xFF;

	typedef std::vector<Key>::iterator kit_t;
	typedef std::vector<Device>::iterator dit_t;
	for (kit_t it = _keys.begin(); it != _keys.end(); ++it) {
		int id_diff = it->lookup_desc().id() - epoch_start;

		if (id_diff < 0)
			id_diff += 256;
		if (id_diff >= skipped)
			continue;

		for (dit_t jt = _devices.begin(); jt != _devices.end(); ++jt) {
			jt->keys().erase(it->lookup_desc());
			if (jt->keys().size() == 0)
				_devices.erase(jt);
		}

		_keys.erase(it);
	}

	_key_epoch = to_epoch;
}

void Network::add_keys_until(uint64_t to_epoch)
{
	if (to_epoch - _key_epoch >= 256)
		throw std::runtime_error("maximum number of keys exceeded");

	uint8_t epoch_start = _key_epoch & 0xFF;
	for (unsigned id_off = 0; id_off < to_epoch - _key_epoch; id_off++) {
		if (find_key(kld(epoch_start + id_off)) != _keys.end())
			continue;

		boost::array<uint8_t, 16> key = derive_key(_key_epoch + id_off);
		_keys.push_back(
			Key::indexed(
				KEY_FRAME_TYPES,
				KEY_CMD_FRAME_IDS,
				key,
				epoch_start + id_off));

		BOOST_FOREACH(Device& dev, _devices) {
			dev.keys()[_keys.back().lookup_desc()] = 0;
		}
	}
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
	FD tfm(socket(AF_ALG, SOCK_SEQPACKET, 0));

	if (tfm < 0)
		throw std::runtime_error("could not open crypto socket");

	struct sockaddr_alg algdesc = {
		PF_ALG,
		"skcipher",
		0,
		0,
		"ecb(aes)"
	};

	if (bind(tfm, reinterpret_cast<const sockaddr*>(&algdesc), sizeof(algdesc)) < 0)
		throw std::runtime_error(std::string("bind: ") + strerror(errno));

	const int SOL_ALG = 279;

	if (setsockopt(tfm, SOL_ALG, ALG_SET_KEY, &_master_key[0], _master_key.size()) < 0)
		throw std::runtime_error(std::string("setsockopt: ") + strerror(errno));

	FD req(accept(tfm, NULL, 0));
	if (req < 0)
		throw std::runtime_error(std::string("accept: ") + strerror(errno));

	boost::array<uint8_t, 16> buf = convert_key_id(id);

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
	iov.iov_base = &buf[0];
	iov.iov_len = buf.size();

	if (sendmsg(req, &msg_out, 0) < 0)
		throw std::runtime_error(std::string("sendmsg: ") + strerror(errno));

	if (read(req, buf.begin(), buf.size()) < 0)
		throw std::runtime_error(std::string("read: ") + strerror(errno));

	return buf;
}



namespace {

void save(BinaryFormatter& fmt, const KeyLookupDescriptor& desc)
{
	fmt.put_u8(desc.mode());
	fmt.put_u8(desc.id());
}

KeyLookupDescriptor load_kld(BinaryFormatter& fmt)
{
	uint8_t mode = fmt.get_u8();
	uint8_t id = fmt.get_u8();

	return KeyLookupDescriptor(mode, id);
}



void save(BinaryFormatter& fmt, const SecurityParameters& params)
{
	fmt.put_u8(params.enabled());
	fmt.put_u8(params.out_level());
	save(fmt, params.out_key());
	fmt.put_u32(params.frame_counter());
}

SecurityParameters load_secparams(BinaryFormatter& fmt)
{
	bool enabled = fmt.get_u8();
	uint8_t out_level = fmt.get_u8();
	KeyLookupDescriptor out_key = load_kld(fmt);
	uint32_t frame_counter = fmt.get_u32();

	return SecurityParameters(enabled, out_level, out_key, frame_counter);
}



void save(BinaryFormatter& fmt, const Device& dev)
{
	fmt.put_u16(dev.short_addr());
	fmt.put_u64(dev.hwaddr());

	typedef std::pair<KeyLookupDescriptor, uint32_t> p_t;
	uint8_t key_count = 0;
	BOOST_FOREACH(const p_t& devkey, dev.keys()) {
		if (devkey.second)
			key_count++;
	}

	fmt.put_u8(key_count);
	BOOST_FOREACH(const p_t& devkey, dev.keys()) {
		if (devkey.second) {
			fmt.put_u8(devkey.first.id());
			fmt.put_u32(devkey.second);
		}
	}
}

Device load_device(BinaryFormatter& fmt, uint16_t pan_id, uint8_t devkey_mode)
{
	uint16_t short_addr = fmt.get_u16();
	uint64_t hwaddr = fmt.get_u64();

	Device result(pan_id, short_addr, hwaddr, 0, false, devkey_mode);

	uint8_t devkeys = fmt.get_u8();
	while (devkeys-- > 0) {
		uint8_t idx = fmt.get_u8();
		uint32_t ctr = fmt.get_u32();

		result.keys().insert(std::make_pair(kld(idx), ctr));
	}

	return result;
}

}

void Network::save(Eeprom& target)
{
	std::vector<uint8_t> stream;
	BinaryFormatter fmt(stream);

	fmt.put_u8(VERSION_1);

	fmt.put_u16(_pan.pan_id());
	fmt.put_u8(_pan.page());
	fmt.put_u8(_pan.channel());
	fmt.put_u16(_short_addr);
	fmt.put_u64(_hwaddr);
	fmt.put_u64(_key_epoch);
	::save(fmt, _sec_params);
	fmt.put_raw(_master_key.c_array(), 16);

	fmt.put_u8(_keys.size());

	fmt.put_u8(_devices.size());
	BOOST_FOREACH(const Device& dev, _devices) {
		::save(fmt, dev);
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
		uint64_t key_epoch = fmt.get_u64();
		SecurityParameters params = load_secparams(fmt);
		boost::array<uint8_t, 16> master_key;

		fmt.get_raw(master_key.c_array(), 16);

		Network result(PAN(pan_id, page, channel), short_addr,
				hwaddr, params, master_key);

		result._key_epoch = key_epoch;

		uint8_t keys = fmt.get_u8();

		uint8_t devices = fmt.get_u8();
		while (devices-- > 0) {
			Device dev(load_device(fmt, pan_id, DEVICE_KEY_MODE));

			result._devices.push_back(dev);
		}

		result.add_keys_until(result.key_epoch() + keys - 1);

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
