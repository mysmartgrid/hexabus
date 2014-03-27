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
#include "eeprom.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

static const int DEFAULT_PAN_PAGE = 2;
static const int DEFAULT_PAN_CHANNEL = 0;

void teardown_all()
{
	Controller ctrl;

	std::vector<NetDevice> netdevs;
	do {
		netdevs = ctrl.list_netdevs();

		BOOST_FOREACH(const NetDevice& dev, netdevs) {
			try {
				ctrl.remove_netdev(dev.name());
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

	BOOST_FOREACH(const Key& key, net.keys()) {
		ctrl.add_key(wpan.name(), key);
	}
	BOOST_FOREACH(const Device& dev, net.devices()) {
		ctrl.add_device(wpan.name(), dev);
	}
	ctrl.setparams(
		wpan.name(),
		SecurityParameters(true, 5, net.out_key(), net.frame_counter()));
	ctrl.add_seclevel(wpan.name(), Seclevel(1, 0xff));

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

Network extract_network(const std::string& iface)
{
	Controller ctrl;

	NetDevice netdev = ctrl.list_netdevs(iface).at(0);
	SecurityParameters sp = ctrl.getparams(iface);

	Network result(PAN(netdev.pan_id(), DEFAULT_PAN_PAGE, DEFAULT_PAN_CHANNEL),
			netdev.short_addr(), netdev.addr(), sp.out_key(),
			sp.frame_counter());

	std::vector<Key> keys = ctrl.list_keys(iface);
	BOOST_FOREACH(const Key& key, keys) {
		result.add_key(key);
	}

	std::vector<Device> devices = ctrl.list_devices(iface);
	BOOST_FOREACH(const Device& dev, devices) {
		result.add_device(dev);
	}

	return result;
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

	{
		uint32_t ctr = 0;
		Controller ctrl;

		SecurityParameters p = ctrl.getparams(dev);
		ctr = p.frame_counter();
		BOOST_FOREACH(const Device& d, ctrl.list_devices(dev)) {
			ctr = std::max(d.frame_ctr(), ctr);
		}
		SecurityParameters np(p.enabled(), p.out_level(), p.out_key(),
				ctr + 10000000);
		ctrl.setparams(dev, np);
	}

	resync.force();
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

void dump_params(const std::string& iface)
{
	SecurityParameters params = Controller().getparams(iface);

	std::cout << params << std::endl;
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
	} else if (cmd == "list-params") {
		dump_params(argc > 2 ? argv[2] : "");
	} else if (cmd == "list") {
		if (argc < 3) {
			std::cerr << "required args: <iface>" << std::endl;
			return 1;
		}
		dump_keys(argv[2]);
		dump_devices(argv[2]);
		dump_params(argv[2]);
	} else if (cmd == "eep") {
		Eeprom eep("/dev/i2c-1");

		eep.dump_contents();

		std::vector<uint8_t> s;
		for (int i = 0; i < 64; i++)
			s.push_back(i);

		eep.write_stream(s);
	} else {
		std::cerr << "Unknown command" << std::endl;
		return 1;
	}

	return 0;
}
