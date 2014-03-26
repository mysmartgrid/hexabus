#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <string>
#include <vector>
#include <string.h>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

class KeyLookupDescriptor {
	private:
		uint8_t _mode;
		uint8_t _id;
		boost::variant<uint32_t, uint64_t> _source;

	public:
		KeyLookupDescriptor(uint8_t mode, uint8_t id = 0,
			boost::variant<uint32_t, uint64_t> source = boost::variant<uint32_t, uint64_t>())
			: _mode(mode), _id(id), _source(source)
		{}

		uint8_t mode() const { return _mode; }
		uint8_t id() const { return _id; }
		boost::variant<uint32_t, uint64_t> source() const { return _source; }
};

inline bool operator==(const KeyLookupDescriptor& a, const KeyLookupDescriptor& b)
{
	return a.mode() == b.mode() &&
		(a.mode() == 0 ||
			(a.id() == b.id() &&
				(a.mode() == 1 || a.source() == b.source())));
}

inline bool operator!=(const KeyLookupDescriptor& a, const KeyLookupDescriptor& b)
{
	return !(a == b);
}

class Key {
	private:
		std::string _iface;
		KeyLookupDescriptor _lookup_desc;
		uint8_t _frame_types;
		uint32_t _cmd_frame_ids;
		uint8_t _key[16];

		Key(const std::string& iface, const KeyLookupDescriptor lookup_desc,
			uint8_t frame_types, uint32_t cmd_frame_ids,
			const uint8_t* key)
			: _iface(iface), _lookup_desc(lookup_desc),
			  _frame_types(frame_types), _cmd_frame_ids(cmd_frame_ids)
		{
			memcpy(_key, key, 16);
		}

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
		const KeyLookupDescriptor& lookup_desc() const { return _lookup_desc; }
		uint8_t frame_types() const { return _frame_types; }
		uint32_t cmd_frame_ids() const { return _cmd_frame_ids; }
		const uint8_t* key() const { return _key; }
};

class Device {
	private:
		std::string _iface;
		uint16_t _pan_id;
		uint16_t _short_addr;
		uint64_t _hwaddr;
		uint32_t _frame_ctr;
		KeyLookupDescriptor _key;

	public:
		Device(const std::string& iface, uint64_t hwaddr, uint32_t frame_ctr,
			const KeyLookupDescriptor& key)
			: _iface(iface), _pan_id(0), _short_addr(0xfffe), _hwaddr(hwaddr),
			  _frame_ctr(frame_ctr), _key(key)
		{}

		Device(const std::string& iface, uint16_t pan_id, uint16_t short_addr,
			uint64_t hwaddr, uint32_t frame_ctr, const KeyLookupDescriptor& key)
			: _iface(iface), _pan_id(pan_id), _short_addr(short_addr),
			  _hwaddr(hwaddr), _frame_ctr(frame_ctr), _key(key)
		{}

		const std::string& iface() const { return _iface; }
		uint16_t pan_id() const { return _pan_id; }
		uint16_t short_addr() const { return _short_addr; }
		uint64_t hwaddr() const { return _hwaddr; }
		uint32_t frame_ctr() const { return _frame_ctr; }
		const KeyLookupDescriptor& key() const { return _key; }
};

class NetDevice {
	private:
		std::string _phy;
		std::string _name;
		boost::optional<uint64_t> _addr;
		boost::optional<uint16_t> _short_addr;
		boost::optional<uint16_t> _pan_id;

	public:
		NetDevice(const std::string& phy, const std::string& name)
			: _phy(phy), _name(name)
		{}

		NetDevice(const std::string& phy, const std::string& name, uint64_t addr,
				uint16_t short_addr, uint16_t pan_id)
			: _phy(phy), _name(name), _addr(addr), _short_addr(short_addr),
			  _pan_id(pan_id)
		{}

		const std::string& phy() const { return _phy; }
		const std::string& name() const { return _name; }
		uint64_t addr() const { return *_addr; }
		uint16_t short_addr() const { return *_short_addr; }
		uint16_t pan_id() const { return *_pan_id; }
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

class PAN {
	private:
		uint16_t _pan_id;
		int _page;
		int _channel;

	public:
		PAN(uint16_t id, int page, int channel)
			: _pan_id(id), _page(page), _channel(channel)
		{}

		uint16_t pan_id() const { return _pan_id; }
		int page() const { return _page; }
		int channel() const { return _channel; }
};

#endif
