#ifndef COMMANDS_HPP_
#define COMMANDS_HPP_

#include <string>
#include <vector>

#include "netlink.hpp"
#include "types.hpp"

struct Controller : private nl::socket {
	Controller();

	typedef std::vector<std::pair<Device, std::string> > dev_list_t;
	typedef std::vector<std::pair<Key, std::string> > key_list_t;
	typedef std::vector<std::pair<Seclevel, std::string> > sl_list_t;

	std::vector<Phy> list_phys(const std::string& name = "");
	std::vector<NetDevice> list_netdevs(const std::string& iface = "");
	dev_list_t list_devices();
	std::vector<Device> list_devices(const std::string& iface);
	std::vector<Device> list_devices(const std::string& iface, uint64_t addr);
	key_list_t list_keys();
	std::vector<Key> list_keys(const std::string& iface);
	sl_list_t list_seclevels();
	std::vector<Seclevel> list_seclevels(const std::string& iface);

	void setup_dev(const std::string& dev);
	void start(const std::string& dev, const PAN& pan, uint16_t short_addr = 0xfffe);
	NetDevice add_netdev(const Phy& phy, uint64_t addr, const std::string& name = "");
	NetDevice add_monitor(const std::string& phy, const std::string& name = "");
	void remove_netdev(const std::string& name);
	Key get_out_key(const std::string& iface);

	void setparams(const std::string& dev, const SecurityParameters& params);
	SecurityParameters getparams(const std::string& dev);

	void add_seclevel(const std::string& dev, const Seclevel& sl);
	void add_key(const std::string& iface, const Key& key);
	void add_device(const std::string& iface, const Device& dev);
	void add_device_key(const std::string& iface, uint64_t hwaddr,
			const std::pair<KeyLookupDescriptor, uint32_t>& key);

	void remove_device(const std::string& iface, uint64_t hwaddr);
	void remove_device_key(const std::string& iface, uint64_t hwaddr,
			const KeyLookupDescriptor& key);
	void remove_key(const std::string& iface, const Key& key);
};

void create_lowpan_device(const std::string& master, const std::string& lowpan);

#endif