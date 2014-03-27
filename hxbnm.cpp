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

int teardown_all()
{
	Controller ctrl;

	std::vector<NetDevice> netdevs;
	do {
		netdevs = ctrl.list_netdevs();

		BOOST_FOREACH(const NetDevice& dev, netdevs) {
			try {
				ctrl.remove_netdev(dev.name());
			} catch (const nl::nl_error& e) {
				if (e.error() != NLE_NODEV) {
					std::cerr << "teardown: " << e.what() << std::endl;
					return 1;
				}
			}
		}
	} while (netdevs.size());

	return 0;
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

int save_eeprom(const std::string& file, const std::string& iface)
{
	Eeprom eep(file);
	Network net = extract_network(iface);

	net.save(eep);
	eep.dump_contents();
	return 0;
}

Network load_eeprom(const std::string& file)
{
	Eeprom eep(file);

	return Network::load(eep);
}

int setup_network(const Network& net)
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

	return 0;
}

int setup(const std::string& file)
{
	Network old = load_eeprom(file);

	return setup_network(old);
}

int setup_random(const std::string& file)
{
	Network old = load_eeprom(file);
	Network next = Network::random(old.hwaddr());

	return setup_network(next);
}

int setup_random_full()
{
	Network next = Network::random();

	return setup_network(next);
}

int run_pairing(const std::string& iface)
{
	Controller ctrl;

	NetDevice dev = ctrl.list_netdevs(iface).at(0);

	PairingHandler handler(iface, dev.pan_id());

	try {
		handler.run_once();
	} catch (const std::exception& e) {
		std::cerr << "pair: " << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "pair" << std::endl;
		return 1;
	}

	return 0;
}

int run_resync(const std::string& dev)
{
	ResyncHandler resync(dev);

	try {
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

		resync.force();
	} catch (const std::exception& e) {
		std::cerr << "resynd: " << e.what() << std::endl;
		return 1;
	}

	while (true) {
		try {
			resync.run_once();
		} catch (const std::exception& e) {
			std::cerr << "resyncd: " << e.what() << std::endl;
			return 1;
		} catch (...) {
			std::cerr << "resyncd failed" << std::endl;
			return 1;
		}
	}
}

int dump_phys()
{
	std::vector<Phy> phys = Controller().list_phys();

	for (size_t i = 0; i < phys.size(); i++) {
		std::cout << phys[i] << std::endl;
	}

	return 0;
}

int dump_keys(const std::string& iface = "")
{
	std::vector<Key> keys = Controller().list_keys(iface);

	for (size_t i = 0; i < keys.size(); i++)
		std::cout << keys[i] << std::endl;

	return 0;
}

int dump_devices(const std::string& iface = "")
{
	std::vector<Device> devs = Controller().list_devices(iface);

	for (size_t i = 0; i < devs.size(); i++)
		std::cout << devs[i] << std::endl;

	return 0;
}

int dump_params(const std::string& iface)
{
	SecurityParameters params = Controller().getparams(iface);

	std::cout << params << std::endl;

	return 0;
}

enum {
	C_HELP,
	C_TEARDOWN_ALL,
	C_SETUP,
	C_PAIR,
	C_RESYNCD,
	C_LIST_KEYS,
	C_LIST_DEVICES,
	C_LIST_PARAMS,
	C_LIST,
	C_LIST_PHYS,
	C_SAVE_EEPROM,

	C_MAX_COMMAND__,
};

static const char* commands[] = {
	"help",
	"teardown-all",
	"setup",
	"pair",
	"resyncd",
	"list-keys",
	"list-devices",
	"list-params",
	"list",
	"list-phys",
	"save-eeprom",
};

int parse_command(const std::string& cmd)
{
	for (int i = 0; i < C_MAX_COMMAND__; i++) {
		if (cmd == commands[i]) {
			return i;
		}
	}

	return -1;
}

void help(std::ostream& os)
{
	os << "Usage: <command> [args]" << std::endl
		<< "help                    show this help" << std::endl
		<< "teardown-all            tear down all WPANs and associated devices" << std::endl
		<< "setup                   set up a WPAN and cryptographic state from EEPROM" << std::endl
		<< "  random                generate random network, use MAC from EEPROM" << std::endl
		<< "  random-full           same, but don't reuse MAC address from last network" << std::endl
		<< "pair <iface>            pair one device to <iface>" << std::endl
		<< "resyncd <iface>         run resync process on <iface>" << std::endl
		<< "list-keys               list WPAN keys on the system" << std::endl
		<< "  [iface]               show only keys on iface" << std::endl
		<< "list-devices            list paired devices on the system" << std::endl
		<< "  [iface]               show only devices on iface" << std::endl
		<< "list-params <iface>     show security parameters of iface" << std::endl
		<< "list <iface>            list key, devices and security parameters" << std::endl
		<< "list-phys               list WPAN phys on the system" << std::endl
		<< "save-eeprom <iface>     save network on iface to EEPROM" << std::endl;
}

class no_arg {};
class ArgParser {
	private:
		int idx;
		int argc;
		const char** argv;

	public:
		ArgParser(int argc, const char** argv)
			: idx(1), argc(argc), argv(argv)
		{}

		bool more() const
		{
			return idx < argc;
		}

		boost::optional<std::string> maybe()
		{
			if (!more())
				return boost::optional<std::string>();

			return boost::make_optional(std::string(argv[idx++]));
		}

		std::string operator()()
		{
			boost::optional<std::string> v = maybe();

			if (!v)
				throw no_arg();

			return *v;
		}

		std::string operator()(const std::string& def)
		{
			boost::optional<std::string> v = maybe();

			return v ? *v : def;
		}
};

int main(int argc, const char* argv[])
{
	static const char* EEP_FILE = "/dev/i2c-1";

	if (argc == 1) {
		help(std::cerr);
		return 1;
	}

	ArgParser next(argc, argv);
	int command = parse_command(next());

	try {
		switch (command) {
		case C_HELP:
			help(std::cout);
			return 0;

		case C_TEARDOWN_ALL:
			return teardown_all();

		case C_SETUP: {
			boost::optional<std::string> random = next.maybe();
			if (random) {
				if (next.more())
					throw no_arg();

				if (*random == "random") {
					return setup_random(EEP_FILE);
				} else if (*random == "random-full") {
					return setup_random_full();
				} else {
					throw no_arg();
				}
			} else {
				return setup(EEP_FILE);
			}
		}

		case C_PAIR:
			return run_pairing(next());

		case C_RESYNCD:
			return run_resync(next());

		case C_LIST_KEYS:
			return dump_keys(next(""));

		case C_LIST_DEVICES:
			return dump_devices(next(""));

		case C_LIST_PARAMS:
			return dump_params(next());

		case C_LIST: {
			std::string iface = next();
			return dump_keys(iface) || dump_devices(iface) ||
				dump_params(iface);
		}

		case C_LIST_PHYS:
			return dump_phys();

		case C_SAVE_EEPROM:
			return save_eeprom("/dev/i2c-1", next());
		}
	} catch (const no_arg& e) {
		std::cerr << "Command line invalid" << std::endl;
		help(std::cerr);
		return 1;
	} catch (const std::exception& e) {
		std::cerr << "Command failed: " << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "Command failed" << std::endl;
		return 1;
	}

	return 0;
}
