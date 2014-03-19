#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <iostream>
#include <string>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <boost/format.hpp>
#include <boost/variant.hpp>

class Device {
	private:
		std::string _iface;
		uint16_t _pan_id;
		uint16_t _short_addr;
		uint64_t _hwaddr;
		uint32_t _frame_ctr;

	public:
		Device(const std::string& iface, uint64_t hwaddr, uint32_t frame_ctr)
			: _iface(iface), _pan_id(0), _short_addr(0xfffe), _hwaddr(hwaddr), _frame_ctr(frame_ctr)
		{}

		Device(const std::string& iface, uint16_t pan_id, uint16_t short_addr, uint64_t hwaddr, uint32_t frame_ctr)
			: _iface(iface), _pan_id(pan_id), _short_addr(short_addr), _hwaddr(hwaddr), _frame_ctr(frame_ctr)
		{}

		const std::string& iface() const { return _iface; }
		uint16_t pan_id() const { return _pan_id; }
		uint16_t short_addr() const { return _short_addr; }
		uint64_t hwaddr() const { return _hwaddr; }
		uint32_t frame_ctr() const { return _frame_ctr; }
};

inline std::string format_mac(uint64_t addr)
{
	char buf[32];
	char mac[8];

	memcpy(mac, &addr, 8);
	snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]);
	return buf;
}

inline std::string bitlist(uint32_t bits)
{
	std::stringstream ss;

	for (int i = 0; bits != 0; i++, bits >>= 1) {
		if (i)
			ss << " ";
		if (bits & 1)
			ss << i;
	}

	return ss.str();
}

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Device& dev)
{
	using boost::format;

	os << format("Device on %1%") % dev.iface() << std::endl
		<< format("	PAN ID %|04x|") % dev.pan_id() << std::endl
		<< format("	Short address %|04x|") % dev.short_addr() << std::endl
		<< format("	Extended address %1%") % format_mac(dev.hwaddr()) << std::endl
		<< format("	Frame counter %1%") % dev.frame_ctr() << std::endl;

	return os;
}

class NetDevice {
	private:
		std::string _phy;
		std::string _name;

	public:
		NetDevice(const std::string& phy, const std::string& name)
			: _phy(phy), _name(name)
		{}

		const std::string& phy() const { return _phy; }
		const std::string& name() const { return _name; }
};

struct phy {
	std::string name;
	uint32_t page, channel;
	std::vector<uint32_t> pages;
	int txpower, cca_mode, ed_level;
	bool lbt, aack, retransmit;
};

class Phy {
	private:
		std::string _name;
		uint32_t _page;
		uint32_t _channel;
		std::vector<uint32_t> _supported_pages;
		int _txpower;
		uint8_t _cca_mode;
		int _ed_level;
		bool _lbt;

	public:
		Phy(const std::string& name, uint32_t page, uint32_t channel,
			const std::vector<uint32_t>& supported_pages)
			: _name(name), _page(page), _channel(channel),
			  _supported_pages(supported_pages)
		{}

		Phy(const std::string& name, uint32_t page, uint32_t channel,
			const std::vector<uint32_t>& supported_pages,
			int txpower, uint8_t cca_mode, int ed_level, bool lbt)
			: _name(name), _page(page), _channel(channel),
			  _supported_pages(supported_pages),
			  _txpower(txpower), _cca_mode(cca_mode),
			  _ed_level(ed_level), _lbt(lbt)
		{}

		const std::string& name() const { return _name; }
		uint32_t page() const { return _page; }
		uint32_t channel() const { return _channel; }
		const std::vector<uint32_t>& supported_pages() const { return _supported_pages; }
		int txpower() const { return _txpower; }
		uint8_t cca_mode() const { return _cca_mode; }
		int ed_level() const { return _ed_level; }
		bool lbt() const { return _lbt; }
};

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Phy& phy)
{
	using boost::format;

	os << format("PHY %1%") % phy.name() << std::endl
		<< format("	Channel %1%") % phy.channel() << std::endl
		<< format("	Page %1%") % phy.page() << std::endl
		<< format("	Transmit power %1%") % phy.txpower() << std::endl
		<< format("	LBT %1%") % (phy.lbt() ? "on" : "off") << std::endl
		<< format("	CCA mode %|i|") % phy.cca_mode() << std::endl
		<< format("	CCA ED level %1%") % phy.ed_level() << std::endl;

	typedef std::vector<uint32_t>::const_iterator iter;
	int i = 0;
	for (iter it = phy.supported_pages().begin(),
		end = phy.supported_pages().end();
	     it != end;
	     it++, i++) {
		int page = *it;
		if (page != 0) {
			os << format("	Channels on page %1%: %2%") % i % bitlist(page) << std::endl;
		}
	}

	return os;
}

class Key {
	private:
		std::string _iface;
		uint8_t _mode;
		uint8_t _id;
		uint8_t _frame_types;
		uint32_t _cmd_frame_ids;
		boost::variant<uint32_t, uint64_t> _source;
		uint8_t _key[16];

		Key() {}

	public:
		static Key implicit(const std::string& iface, uint8_t frames,
			uint8_t cmd_frames, const uint8_t* key);

		static Key indexed(const std::string& iface, uint8_t frames,
			uint8_t cmd_frames, const uint8_t* key, uint8_t id);

		static Key indexed(const std::string& iface, uint8_t frames,
			uint8_t cmd_frames, const uint8_t* key, uint8_t id,
			uint32_t source);

		static Key indexed(const std::string& iface, uint8_t frames,
			uint8_t cmd_frames, const uint8_t* key, uint8_t id,
			uint64_t source);

		const std::string& iface() const { return _iface; }
		uint8_t mode() const { return _mode; }
		uint8_t id() const { return _id; }
		uint8_t frame_types() const { return _frame_types; }
		uint32_t cmd_frame_ids() const { return _cmd_frame_ids; }
		boost::variant<uint32_t, uint64_t> source() const { return _source; }
		const uint8_t* key() const { return _key; }
};

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Key& key)
{
	using boost::format;

	os << format("Key on %1%") % key.iface() << std::endl
		<< format("	Mode %%1") % int(key.mode()) << std::endl;

	if (key.mode() != 0) {
		if (key.mode() == 2) {
			uint32_t source = boost::get<uint32_t>(key.source());
			uint16_t pan = source >> 16;
			uint16_t saddr = source & 0xFFFF;

			os << format("	Source %|04x|:%|04x|") % pan % saddr << std::endl;
		} else if (key.mode() == 3) {
			uint64_t source = boost::get<uint64_t>(key.source());

			os << format("	Source %1%") % format_mac(source) << std::endl;
		}

		os << format("	Index %1%") % int(key.id()) << std::endl;
	}

	os << format("	Frame types %1%") % bitlist(key.frame_types()) << std::endl;
	if (key.frame_types() & (1 << 3))
		os << format("	Command frames %1%") % bitlist(key.cmd_frame_ids()) << std::endl;

	os << "	Key ";
	for (int i = 0; i < 16; i++) {
		os << format(" %|02x|") % int(key.key()[i]);
	}
	os << std::endl;

	return os;
}

class Seclevel {
	private:
		std::string _iface;
		uint8_t _frame_type;
		uint8_t _levels;

	public:
		Seclevel(const std::string& iface, uint8_t frame_type, uint8_t levels)
			: _iface(iface), _frame_type(frame_type), _levels(levels)
		{}

		const std::string& iface() const { return _iface; }
		uint8_t frame_type() const { return _frame_type; }
		uint8_t levels() const { return _levels; }
};

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Seclevel& sl)
{
	using boost::format;

	os << format("Security lveel on %1%") % sl.iface() << std::endl
		<< format("	Frame type %|i|") % sl.frame_type() << std::endl
		<< format("	Levels %1%") % bitlist(sl.levels()) << std::endl;

	return os;
}

#endif
