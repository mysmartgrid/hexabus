#ifndef COMMANDS_HPP_
#define COMMANDS_HPP_

#include <string>
#include <vector>

#include "netlink.hpp"
#include "types.hpp"

struct Controller : private nl::socket {
	Controller();

	std::vector<Phy> list_phys();
	std::vector<NetDevice> list_netdevs(const std::string& iface = "");
	std::vector<Device> list_devices(const std::string& iface = "");
	std::vector<Device> list_devices(const std::string& iface, uint64_t addr);
	std::vector<Key> list_keys(const std::string& iface = "");

	void setup_phy(const std::string& phy);
	void start(const std::string& dev, const PAN& pan, uint16_t short_addr = 0xfffe);
	NetDevice add_netdev(const Phy& phy, uint64_t addr, const std::string& name = "");
	void remove_netdev(const std::string& name);
	Key get_out_key(const std::string& iface);

	void setparams(const std::string& dev, const SecurityParameters& params);
	SecurityParameters getparams(const std::string& dev);

	void add_seclevel(const std::string& dev, const Seclevel& sl);
	void add_key(const std::string& iface, const Key& key);
	void add_device(const Device& dev);
};

#endif
