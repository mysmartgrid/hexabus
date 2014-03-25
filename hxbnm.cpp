#include <errno.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <limits>
#include <stdint.h>
#include <endian.h>
#include <fcntl.h>

#include "ieee802154.h"
#include "nl802154.h"

#include "netlink.hpp"
#include "nlmessages.hpp"
#include "nlparsers.hpp"
#include "types.hpp"
#include "types_io.hpp"
#include "controller.hpp"
#include "bootstrap.hpp"
#include "network.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

void teardown_all()
{
	Controller ctrl;

	std::vector<NetDevice> netdevs;
	do {
		netdevs = ctrl.list_netdevs();

		for (size_t i = 0; i < netdevs.size(); i++) {
			try {
				ctrl.remove_netdev(netdevs[i].name());
			} catch (const nl::nl_error& e) {
				if (e.error() != NLE_NODEV)
					throw;
			}
		}
	} while (netdevs.size());
}

std::string setup_network(const Network& net)
{
	Controller ctrl;
	Phy phy = ctrl.list_phys().at(0);

	NetDevice wpan = ctrl.add_netdev(phy, net.hwaddr());

	ctrl.start(wpan.name(), net.pan());
	ctrl.setup_phy(phy.name());

	typedef std::vector<Key>::const_iterator key_cit;
	typedef std::map<Device, Key>::const_iterator dev_cit;

	for (key_cit it = net.keys().begin(), end = net.keys().end(); it != end; it++) {
		ctrl.add_key(wpan.name(), *it);
	}
	for (dev_cit it = net.devices().begin(), end = net.devices().end(); it != end; it++) {
		ctrl.add_device(it->first);
	}
	ctrl.enable_security(wpan.name(), net.out_key());

	if (system((boost::format("ip link set %1% up") % wpan.name()).str().c_str()))
		throw std::runtime_error("can't create device");

	std::string link_name;
	for (int i = 0; ; i++) {
		link_name = (boost::format("usb%1%") % i).str();

		std::string link_cmd = (boost::format("ip link add link %1% name %2% type lowpan")
				% wpan.name() % link_name).str();
		if (!system(link_cmd.c_str()))
			break;
	}

	std::string up_cmd = (boost::format("ip link set %1% up") % link_name).str();
	if (system(up_cmd.c_str()))
		throw std::runtime_error("can't create device");

	return wpan.name();
}

void run_pairing(const std::string& iface)
{
	Controller ctrl;

	NetDevice dev = ctrl.list_netdevs(iface).at(0);

	PairingHandler handler(iface, dev.pan_id());

	try {
		handler.run_once();
	} catch (const std::exception& e) {
		std::cerr << "Bootstrap error: " << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Bootstrap error" << std::endl;
	}
}

void run_resync(const std::string& dev)
{
	ResyncHandler resync(dev);

	while (true) {
		try {
			resync.run_once();
		} catch (const std::exception& e) {
			std::cerr << "Bootstrap error: " << e.what() << std::endl;
		} catch (...) {
			std::cerr << "Bootstrap error" << std::endl;
		}
	}
}

void dump_phys()
{
	std::vector<Phy> phys = Controller().list_phys();

	for (size_t i = 0; i < phys.size(); i++) {
		std::cout << phys[i] << std::endl;
	}
}

void dump_keys(const std::string& iface = "")
{
	std::vector<Key> keys = Controller().list_keys(iface);

	for (size_t i = 0; i < keys.size(); i++)
		std::cout << keys[i] << std::endl;
}

void dump_devices(const std::string& iface = "")
{
	std::vector<Device> devs = Controller().list_devices(iface);

	for (size_t i = 0; i < devs.size(); i++)
		std::cout << devs[i] << std::endl;
}

int main(int argc, const char* argv[])
{
	if (argc == 1) {
		std::cerr << "Need command" << std::endl;
		return 1;
	}

	std::string cmd = argv[1];

	if (cmd == "teardown-all") {
		teardown_all();
	} else if (cmd == "setup-random") {
		Network net = Network::random();

		std::string netdev = setup_network(net);

		std::cout << boost::format("Device: %1%") % netdev << std::endl;
	} else if (cmd == "pair") {
		if (argc < 3) {
			std::cerr << "required args: <iface>" << std::endl;
			return 1;
		}
		run_pairing(argv[2]);
	} else if (cmd == "resyncd") {
		if (argc < 3) {
			std::cerr << "required args: <iface>" << std::endl;
			return 1;
		}
		run_resync(argv[2]);
	} else if (cmd == "list-keys") {
		dump_keys(argc > 2 ? argv[2] : "");
	} else if (cmd == "list-phys") {
		dump_phys();
	} else if (cmd == "list-devices") {
		dump_devices(argc > 2 ? argv[2] : "");
	} else {
		std::cerr << "Unknown command" << std::endl;
		return 1;
	}

	return 0;
}
