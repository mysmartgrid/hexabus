#ifndef NETWORK_HPP_
#define NETWORK_HPP_

#include <vector>
#include <map>

#include <stdint.h>
#include <boost/array.hpp>

#include "types.hpp"
#include "eeprom.hpp"

class Network {
	private:
		enum {
			VERSION_FACTORY_NEW = 0,
			VERSION_1 = 1,

			MAX_KEYS = 16,
			MAX_DEVICES = 256,
		};

		PAN _pan;
		uint16_t _short_addr;
		uint64_t _hwaddr;
		std::vector<Key> _keys;
		std::vector<Device> _devices;
		KeyLookupDescriptor _out_key;
		uint32_t _frame_counter;
		uint64_t _key_id;

		std::vector<Key>::iterator find_key(const KeyLookupDescriptor& desc);

	public:
		Network(const PAN& pan, uint16_t short_addr, uint64_t hwaddr,
			const KeyLookupDescriptor& out_key,
			uint32_t frame_counter = 0);

		static Network random(uint64_t hwaddr = 0xFFFFFFFFFFFFFFFFULL);

		void save(Eeprom& target);

		static void init_eeprom(Eeprom& target, uint64_t addr);

		static Network load(Eeprom& source);

		PAN pan() const { return _pan; }
		uint16_t short_addr() const { return _short_addr; }
		uint64_t hwaddr() const { return _hwaddr; }

		const std::vector<Key>& keys() const { return _keys; }
		void add_key(const Key& key);
		void remove_key(const Key& key);

		const std::vector<Device>& devices() const { return _devices; }
		void add_device(const Device& dev);

		const KeyLookupDescriptor& out_key() const { return _out_key; }
		void out_key(const KeyLookupDescriptor& key);

		uint32_t frame_counter() const { return _frame_counter; }
		void frame_counter(uint32_t val) { _frame_counter = val; }

		uint64_t key_id() const { return _key_id; }
		void key_id(uint64_t key_id) { _key_id = key_id; }

		boost::array<uint8_t, 16> derive_key(uint64_t id) const;
};

#endif
