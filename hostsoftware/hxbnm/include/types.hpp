#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <string>
#include <vector>
#include <map>
#include <string.h>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/array.hpp>

#include "ieee802154.h"

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

inline bool operator<(const KeyLookupDescriptor& a, const KeyLookupDescriptor& b)
{
	return a.mode() < b.mode() || (a.mode() == b.mode() &&
		(a.id() < b.id() || (a.id() == b.id() &&
			a.source() < b.source())));
}

class Key {
	private:
		KeyLookupDescriptor _lookup_desc;
		uint8_t _frame_types;
		uint32_t _cmd_frame_ids;
		boost::array<uint8_t, 16> _key;

	public:
		Key(const KeyLookupDescriptor lookup_desc, uint8_t frame_types,
			uint32_t cmd_frame_ids, const boost::array<uint8_t, 16>& key)
			: _lookup_desc(lookup_desc), _frame_types(frame_types),
			  _cmd_frame_ids(cmd_frame_ids), _key(key)
		{}

		static Key implicit(uint8_t frames, uint8_t cmd_frames,
				const boost::array<uint8_t, 16>& key);

		static Key indexed(uint8_t frames, uint8_t cmd_frames,
				const boost::array<uint8_t, 16>& key, uint8_t id);

		static Key indexed(uint8_t frames, uint8_t cmd_frames,
				const boost::array<uint8_t, 16>& key, uint8_t id,
				uint32_t source);

		static Key indexed(uint8_t frames, uint8_t cmd_frames,
				const boost::array<uint8_t, 16>& key, uint8_t id,
				uint64_t source);

		const KeyLookupDescriptor& lookup_desc() const { return _lookup_desc; }
		uint8_t frame_types() const { return _frame_types; }
		uint32_t cmd_frame_ids() const { return _cmd_frame_ids; }
		boost::array<uint8_t, 16> key() const { return _key; }
};

inline bool operator==(const Key& a, const Key& b)
{
	return a.lookup_desc() == b.lookup_desc() &&
		a.frame_types() == b.frame_types() &&
		a.cmd_frame_ids() == b.cmd_frame_ids() &&
		a.key() == b.key();
}

class Device {
	private:
		uint16_t _pan_id;
		uint16_t _short_addr;
		uint64_t _hwaddr;
		uint32_t _frame_ctr;
		bool _sec_override;
		uint8_t _key_mode;
		std::map<KeyLookupDescriptor, uint32_t> _keys;

	public:
		Device(uint16_t pan_id, uint16_t short_addr, uint64_t hwaddr,
				uint32_t frame_ctr, bool sec_override, uint8_t key_mode)
			: _pan_id(pan_id), _short_addr(short_addr),
			  _hwaddr(hwaddr), _frame_ctr(frame_ctr),
			  _sec_override(sec_override), _key_mode(key_mode)
		{}

		uint16_t pan_id() const { return _pan_id; }
		uint16_t short_addr() const { return _short_addr; }
		uint64_t hwaddr() const { return _hwaddr; }
		uint32_t frame_ctr() const { return _frame_ctr; }
		bool sec_override() const { return _sec_override; }
		uint8_t key_mode() const { return _key_mode; }
		const std::map<KeyLookupDescriptor, uint32_t>& keys() const { return _keys; }

		std::map<KeyLookupDescriptor, uint32_t>& keys() { return _keys; }
};

inline bool operator==(const Device& a, const Device& b)
{
	return a.hwaddr() == b.hwaddr();
}

class NetDevice {
	private:
		std::string _phy;
		std::string _name;
		boost::optional<uint64_t> _addr;
		boost::optional<uint16_t> _short_addr;
		boost::optional<uint16_t> _pan_id;
		boost::optional<int> _txpower;
		boost::optional<uint8_t> _cca_mode;
		boost::optional<int> _ed_level;
		boost::optional<bool> _lbt;

	public:
		NetDevice(const std::string& phy, const std::string& name)
			: _phy(phy), _name(name)
		{}

		NetDevice(const std::string& phy, const std::string& name, uint64_t addr,
				uint16_t short_addr, uint16_t pan_id)
			: _phy(phy), _name(name), _addr(addr), _short_addr(short_addr),
			  _pan_id(pan_id)
		{}

		NetDevice(const std::string& phy, const std::string& name, uint64_t addr,
				uint16_t short_addr, uint16_t pan_id,
				int txpower, uint8_t cca_mode, int ed_level, bool lbt)
			: _phy(phy), _name(name), _addr(addr), _short_addr(short_addr),
			  _pan_id(pan_id), _txpower(txpower), _cca_mode(cca_mode),
			  _ed_level(ed_level), _lbt(lbt)
		{}

		const std::string& phy() const { return _phy; }
		const std::string& name() const { return _name; }

		bool has_addrs() const { return _addr;}
		uint64_t addr() const { return *_addr; }
		uint16_t short_addr() const { return *_short_addr; }
		uint16_t pan_id() const { return *_pan_id; }

		ieee802154_addr addr_raw() const;

		bool has_macparams() const { return _txpower; }
		int txpower() const { return *_txpower; }
		uint8_t cca_mode() const { return *_cca_mode; }
		int ed_level() const { return *_ed_level; }
		bool lbt() const { return *_lbt; }
};

class Phy {
	private:
		std::string _name;
		uint32_t _page;
		uint32_t _channel;
		std::vector<uint32_t> _supported_pages;

	public:
		Phy(const std::string& name, uint32_t page, uint32_t channel,
			const std::vector<uint32_t>& supported_pages)
			: _name(name), _page(page), _channel(channel),
			  _supported_pages(supported_pages)
		{}

		const std::string& name() const { return _name; }
		uint32_t page() const { return _page; }
		uint32_t channel() const { return _channel; }
		const std::vector<uint32_t>& supported_pages() const { return _supported_pages; }
};

class Seclevel {
	private:
		uint8_t _frame_type;
		uint8_t _levels;

	public:
		Seclevel(uint8_t frame_type, uint8_t levels)
			: _frame_type(frame_type), _levels(levels)
		{}

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

class SecurityParameters {
	private:
		bool _enabled;
		uint8_t _out_level;
		KeyLookupDescriptor _out_key;
		uint32_t _frame_counter;

	public:
		SecurityParameters(bool enabled, uint8_t out_level,
			const KeyLookupDescriptor& out_key, uint32_t frame_counter)
			: _enabled(enabled), _out_level(out_level),
			  _out_key(out_key), _frame_counter(frame_counter)
		{}

		bool enabled() const { return _enabled; }
		uint8_t out_level() const { return _out_level; }
		const KeyLookupDescriptor& out_key() const { return _out_key; }
		uint32_t frame_counter() const { return _frame_counter; }
};

#endif
