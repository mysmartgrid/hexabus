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

			DEFAULT_PAGE = 2,
			DEFAULT_CHANNEL = 0,
			DEFAULT_SECLEVEL = 5,

			// only allow data frames
			KEY_FRAME_TYPES = 1 << 1,
			KEY_CMD_FRAME_IDS = 0,

			DEVICE_KEY_MODE = IEEE802154_LLSEC_DEVKEY_RESTRICT,

			MAX_DEVICES = 256,
		};

		PAN _pan;
		uint16_t _short_addr;
		uint64_t _hwaddr;
		std::vector<Key> _keys;
		std::vector<Device> _devices;
		SecurityParameters _sec_params;
		boost::array<uint8_t, 16> _master_key;
		uint64_t _key_epoch;

		std::vector<Key>::iterator find_key(const KeyLookupDescriptor& desc);
		boost::array<uint8_t, 16> derive_key(uint64_t id) const;

	public:
		Network(const PAN& pan, uint16_t short_addr, uint64_t hwaddr,
			const SecurityParameters& sec_params,
			const boost::array<uint8_t, 16>& master_key);

		static Network random(uint64_t hwaddr = 0xFFFFFFFFFFFFFFFFULL);

		void save(Eeprom& target);

		static void init_eeprom(Eeprom& target, uint64_t addr);

		static Network load(Eeprom& source);

		PAN pan() const { return _pan; }
		uint16_t short_addr() const { return _short_addr; }
		uint64_t hwaddr() const { return _hwaddr; }

		const std::vector<Key>& keys() const { return _keys; }

		const std::vector<Device>& devices() const { return _devices; }
		const Device& add_device(uint64_t dev_addr);
		void remove_device(const Device& dev);

		template<typename It>
		void replace_devices(const It& begin, const It& end)
		{
			_devices.clear();
			for (It it = begin; it != end; ++it)
				add_device(it->hwaddr());
		}

		const SecurityParameters& sec_params() const { return _sec_params; }
		void sec_params(const SecurityParameters& params);

		const boost::array<uint8_t, 16>& master_key() const { return _master_key; }

		uint64_t key_epoch() const { return _key_epoch; }
		void advance_key_epoch(uint64_t to_epoch);
		void add_keys_until(uint64_t to_epoch);
};

#endif
