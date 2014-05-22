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

static const char* EEP_FILE = "/dev/i2c-1";

int teardown(const std::string& iface = "")
{
	Controller ctrl;
	int loops = 10;

	std::vector<NetDevice> netdevs;
	do {
		try {
			netdevs = ctrl.list_netdevs(iface);
		} catch (const nl::nl_error& e) {
			if (e.error() == NLE_NODEV && !iface.size())
				return 0;
		}

		BOOST_FOREACH(const NetDevice& dev, netdevs) {
			try {
				ctrl.remove_netdev(dev.name());
			} catch (const nl::nl_error& e) {
				if (e.error() != NLE_NODEV) {
					throw;
				}
			}
		}
	} while (netdevs.size() && --loops);

	return !!netdevs.size();
}

Network extract_network(const std::string& iface)
{
	Controller ctrl;

	NetDevice netdev = ctrl.list_netdevs(iface).at(0);
	Phy phy = ctrl.list_phys(netdev.phy()).at(0);
	SecurityParameters sp = ctrl.getparams(iface);

	Network result(PAN(netdev.pan_id(), phy.page(), phy.channel()),
			netdev.short_addr(), netdev.addr(), sp);

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
	return 0;
}

Network load_eeprom(const std::string& file)
{
	Eeprom eep(file);

	return Network::load(eep);
}

int setup_network(const Network& net, const std::string& wpan_name = "",
		const std::string& lowpan = "")
{
	Controller ctrl;
	Phy phy = ctrl.list_phys().at(0);

	NetDevice wpan = ctrl.add_netdev(phy, net.hwaddr(), wpan_name);

	ctrl.start(wpan.name(), net.pan());
	ctrl.setup_dev(wpan.name());

	BOOST_FOREACH(const Key& key, net.keys()) {
		ctrl.add_key(wpan.name(), key);
	}
	BOOST_FOREACH(const Device& dev, net.devices()) {
		ctrl.add_device(wpan.name(), dev);
	}
	ctrl.setparams(wpan.name(), net.sec_params());
	ctrl.add_seclevel(wpan.name(), Seclevel(1, 0xff));

	std::string link_name = lowpan.size() ? lowpan : "hxb%d";
	create_lowpan_device(wpan.name(), link_name);

	return 0;
}

int setup(const std::string& file, const std::string& wpan, const std::string& lowpan)
{
	Network net = load_eeprom(file);

	const SecurityParameters& sp = net.sec_params();
	uint32_t fct = sp.frame_counter();

	BOOST_FOREACH(const Device& dev, net.devices()) {
		fct = std::max(dev.frame_ctr(), fct);
	}

	// on the assumption that 10kk is an upper limit on packet transmissions
	// in the interim
	if (fct != 0)
		fct += 10000000;

	net.sec_params(
		SecurityParameters(
			sp.enabled(), sp.out_level(), sp.out_key(), fct));

	return setup_network(net, wpan, lowpan);
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

template<typename T>
void dump(const std::vector<T>& vec)
{
	BOOST_FOREACH(const T& t, vec)
		std::cout << t << std::endl;
}

int run_pairing(const std::string& iface, int timeout)
{
	Controller ctrl;

	NetDevice dev = ctrl.list_netdevs(iface).at(0);

	PairingHandler handler(iface, dev.pan_id());

	handler.bind(dev.addr_raw());
	handler.run_once(timeout);

	return 0;
}

int dump_phys()
{
	dump(Controller().list_phys());
	return 0;
}

int dump_keys(const std::string& iface = "")
{
	dump(Controller().list_keys(iface));
	return 0;
}

int dump_netdevs(const std::string& iface = "")
{
	dump(Controller().list_netdevs(iface));
	return 0;
}

int dump_devices(const std::string& iface = "")
{
	dump(Controller().list_devices(iface));
	return 0;
}

int dump_seclevels(const std::string& iface = "")
{
	dump(Controller().list_seclevels(iface));
	return 0;
}

int dump_params(const std::string& iface)
{
	SecurityParameters params = Controller().getparams(iface);

	std::cout << params << std::endl;
	return 0;
}

int add_monitor(const std::string& phy, const std::string& name)
{
	NetDevice dev = Controller().add_monitor(phy, name);

	std::cout << dev.name() << std::endl;
	return 0;
}

int init_eeprom(const std::string& file, uint64_t hwaddr)
{
	Eeprom eep(file);

	Network::init_eeprom(eep, hwaddr);
	return 0;
}

int remove_device(const std::string& iface, uint64_t hwaddr)
{
	Controller().remove_device(iface, hwaddr);
	return 0;
}

int dump_eeprom(const std::string& file)
{
	Eeprom eep(file);

	eep.dump_contents();
	return 0;
}

enum {
	C_HELP,
	C_HELP_ALL,
	C_TEARDOWN_ALL,
	C_TEARDOWN,
	C_SETUP,
	C_SETUP_RANDOM,
	C_SETUP_RANDOM_FULL,
	C_PAIR,
	C_LIST_KEYS,
	C_LIST_DEVICES,
	C_LIST_SECLEVELS,
	C_LIST_PARAMS,
	C_LIST_NETDEVS,
	C_LIST,
	C_LIST_PHYS,
	C_SAVE_EEPROM,
	C_MONITOR,
	C_INIT_EEPROM,
	C_REMOVE_DEV,
	C_DUMP_EEPROM,
};

static const struct {
	const char* name;
	int cmd;
} commands[] = {
	{ "help",		C_HELP, },
	{ "help-all",		C_HELP_ALL, },
	{ "teardown-all",	C_TEARDOWN_ALL, },
	{ "teardown",		C_TEARDOWN, },
	{ "setup",		C_SETUP, },
	{ "setup-random",	C_SETUP_RANDOM, },
	{ "setup-random-full",	C_SETUP_RANDOM_FULL, },
	{ "pair",		C_PAIR, },
	{ "list-keys",		C_LIST_KEYS, },
	{ "list-devices",	C_LIST_DEVICES, },
	{ "list-seclevels",	C_LIST_SECLEVELS, },
	{ "list-params",	C_LIST_PARAMS, },
	{ "list-netdevs",	C_LIST_NETDEVS, },
	{ "list",		C_LIST, },
	{ "list-phys",		C_LIST_PHYS, },
	{ "save-eeprom",	C_SAVE_EEPROM, },
	{ "monitor",		C_MONITOR, },
	{ "init-eeprom",	C_INIT_EEPROM, },
	{ "remove-device",	C_REMOVE_DEV, },
	{ "dump-eeprom",	C_DUMP_EEPROM, },

	{ 0, -1 },
};

int parse_command(const std::string& cmd)
{
	for (int i = 0; commands[i].name; i++) {
		if (cmd == commands[i].name) {
			return commands[i].cmd;
		}
	}

	return -1;
}

int help(std::ostream& os, bool all = false)
{
	os << "Usage: <command> [args]" << std::endl
		<< std::endl
		<< "  help                      show this help" << std::endl
		<< "  teardown <iface>          tear down <iface> and associated devices" << std::endl
		<< "  setup                     set up a WPAN and cryptographic state from EEPROM" << std::endl
		<< "    [wpan]                  name the wpan device <wpan>" << std::endl
		<< "    [lowpan]                name the lowpan device <lowpan>" << std::endl
		<< "  setup-random              set up a new WPAN with MAC address from EEPROM" << std::endl
		<< "  pair <iface>              pair one device to <iface>" << std::endl
		<< "    timeout <s>             sets timeout to <s> seconds" << std::endl
		<< "  remove-device             removes a device from the network state" << std::endl
		<< "    <iface>                 interface the device is registered to" << std::endl
		<< "    <mac-addr>              MAC address of the device" << std::endl
		<< "  list-keys                 list WPAN keys on the system" << std::endl
		<< "    [iface]                 show only keys on <iface>" << std::endl
		<< "  list-devices              list paired devices on the system" << std::endl
		<< "    [iface]                 show only devices on <iface>" << std::endl
		<< "  list-seclevels            list security levels of the system" << std::endl
		<< "    [iface]                 show only security levels for <iface>" << std::endl
		<< "  list-netdevs              show WPAN network device info" << std::endl
		<< "    [iface]                 show only <iface>" << std::endl
		<< "  list-params <iface>       show security parameters of <iface>" << std::endl
		<< "  list <iface>              list key, devices, security parameters and netdevs" << std::endl
		<< "  list-phys                 list WPAN phys on the system" << std::endl
		<< "  save-eeprom <iface>       save network on <iface> to EEPROM" << std::endl;

	if (!all)
		return 0;

	os
		<< std::endl
		<< std::endl
		<< "  help-all                  show hidden commands" << std::endl
		<< "  teardown-all              tear down all WPANs and associated devices" << std::endl
		<< "  setup-random-full         like setup-random, but with random MAC" << std::endl
		<< "  monitor <phy> [name]      add a monitor device to phy" << std::endl
		<< "  init-eeprom <MAC>         factory-initialize EEPROM with MAC address" << std::endl
		<< "  dump-eeprom               dump EEPROM contents to console" << std::endl;

	return 0;
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

uint64_t parse_mac(const std::string& str)
{
	const char* cstr = str.c_str();
	uint64_t result = 0;
	int groups = 0;

	while (*cstr) {
		char* end;

		result <<= 8;
		result |= strtoul(cstr, &end, 16);
		groups++;

		if ((groups <= 7 && *end != ':') || (groups == 8 && *end) ||
			!(1 <= end - cstr && end - cstr <= 2)) {
			throw std::invalid_argument("MAC format invalid");
		}

		cstr = *end ? end + 1 : end;
	}

	return htobe64(result);
}

int main(int argc, const char* argv[])
{
	if (argc == 1) {
		help(std::cerr);
		return 1;
	}

	ArgParser next(argc, argv);
	int command = parse_command(next());

	try {
		switch (command) {
		case C_HELP:
			return help(std::cout);

		case C_HELP_ALL:
			return help(std::cout, true);

		case C_TEARDOWN_ALL:
			return teardown();

		case C_TEARDOWN:
			return teardown(next());

		case C_SETUP: {
			std::string wpan = next("");
			std::string lowpan = next("");
			return setup(EEP_FILE, wpan, lowpan);
		}

		case C_SETUP_RANDOM:
			return setup_random(EEP_FILE);

		case C_SETUP_RANDOM_FULL:
			return setup_random_full();

		case C_PAIR: {
			std::string iface = next();
			int timeout = 300;
			if (next.more() && next() == "timeout") {
				timeout = boost::lexical_cast<int>(next());
			}

			return run_pairing(iface, timeout);
		}

		case C_LIST_KEYS:
			return dump_keys(next(""));

		case C_LIST_DEVICES:
			return dump_devices(next(""));

		case C_LIST_SECLEVELS:
			return dump_seclevels(next(""));

		case C_LIST_NETDEVS:
			return dump_netdevs(next(""));

		case C_LIST_PARAMS:
			return dump_params(next());

		case C_LIST: {
			std::string iface = next();
			return dump_keys(iface) || dump_devices(iface) ||
				dump_seclevels(iface) || dump_params(iface) ||
				dump_netdevs(iface);
		}

		case C_LIST_PHYS:
			return dump_phys();

		case C_SAVE_EEPROM:
			return save_eeprom(EEP_FILE, next());

		case C_MONITOR: {
			std::string phy = next();
			std::string name = next("");
			return add_monitor(phy, name);
		}

		case C_INIT_EEPROM:
			return init_eeprom(EEP_FILE, parse_mac(next()));

		case C_REMOVE_DEV: {
			std::string iface = next();
			uint64_t hwaddr = parse_mac(next());
			return remove_device(iface, hwaddr);
		}

		case C_DUMP_EEPROM:
			return dump_eeprom(EEP_FILE);

		default:
			throw no_arg();
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
