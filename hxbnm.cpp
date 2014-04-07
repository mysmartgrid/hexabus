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
#include "resyncd.hpp"

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
	ctrl.setparams(
		wpan.name(),
		SecurityParameters(true, 5, net.out_key(), net.frame_counter()));
	ctrl.add_seclevel(wpan.name(), Seclevel(1, 0xff));

	std::string link_name = lowpan.size() ? lowpan : "hxb%d";
	create_lowpan_device(wpan.name(), link_name);

	return 0;
}

int setup(const std::string& file, const std::string& wpan, const std::string& lowpan)
{
	Network net = load_eeprom(file);

	BOOST_FOREACH(const Device& dev, net.devices()) {
		net.frame_counter(std::max(dev.frame_ctr(), net.frame_counter()));
	}

	// on the assumption that 10kk is an upper limit on packet transmissions
	// in the interim
	if (net.frame_counter() != 0) {
		net.frame_counter(net.frame_counter() + 10000000);
	}

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

int dump_netdevs(const std::string& iface = "")
{
	std::vector<NetDevice> devs = Controller().list_netdevs(iface);

	for (size_t i = 0; i < devs.size(); i++)
		std::cout << devs[i] << std::endl;

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

int add_monitor(const std::string& phy, const std::string& name)
{
	NetDevice dev = Controller().add_monitor(phy, name);

	std::cout << dev.name() << std::endl;
	return 0;
}

int init_eeprom(const std::string& file, const std::string& mac)
{
	std::vector<std::string> parts;

	parts.push_back("");
	BOOST_FOREACH(char c, mac) {
		if (isxdigit(c)) {
			parts.back() += c;
		} else if (c == ':') {
			parts.push_back("");
		} else {
			throw std::runtime_error("incorrect MAC");
		}
	}

	if (parts.size() != 8) {
		throw std::runtime_error("incorrect MAC");
	}

	uint64_t addr = 0;
	BOOST_FOREACH(const std::string& part, parts) {
		if (part.size() > 2) {
			throw std::runtime_error("incorrect MAC");
		}
		addr <<= 8;
		addr |= strtoul(part.c_str(), NULL, 16);
	}

	Eeprom eep(file);

	Network::init_eeprom(eep, htobe64(addr));

	return 0;
}

enum {
	C_HELP,
	C_TEARDOWN_ALL,
	C_TEARDOWN,
	C_SETUP,
	C_SETUP_RANDOM,
	C_SETUP_RANDOM_FULL,
	C_PAIR,
	C_RESYNCD,
	C_LIST_KEYS,
	C_LIST_DEVICES,
	C_LIST_PARAMS,
	C_LIST_NETDEVS,
	C_LIST,
	C_LIST_PHYS,
	C_SAVE_EEPROM,
	C_MONITOR,
	C_INIT_EEPROM,

	C_MAX_COMMAND__,
};

static const char* commands[] = {
	"help",
	"teardown-all",
	"teardown",
	"setup",
	"setup-random",
	"setup-random-full",
	"pair",
	"resyncd",
	"list-keys",
	"list-devices",
	"list-params",
	"list-netdevs",
	"list",
	"list-phys",
	"save-eeprom",
	"monitor",
	"init-eeprom",
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
		<< std::endl
		<< "  help                      show this help" << std::endl
		<< "  teardown-all              tear down all WPANs and associated devices" << std::endl
		<< "  teardown <iface>          tear down <iface> and associated devices" << std::endl
		<< "  setup                     set up a WPAN and cryptographic state from EEPROM" << std::endl
		<< "    [wpan]                  name the wpan device <wpan>" << std::endl
		<< "    [lowpan]                name the lowpan device <lowpan>" << std::endl
		<< "  setup-random              set up a new WPAN with MAC address from EEPROM" << std::endl
		<< "  pair <iface>              pair one device to <iface>" << std::endl
		<< "    timeout <s>             sets timeout to <s> seconds" << std::endl
		<< "  resyncd <iface>           control resync process on <iface>" << std::endl
		<< "    run | run-fg | stop" << std::endl
		<< "      run                   run resync process and daemonize" << std::endl
		<< "      run-fg                run resync process in the foreground" << std::endl
		<< "      stop                  stop a running resync daemon" << std::endl
		<< "  list-keys                 list WPAN keys on the system" << std::endl
		<< "    [iface]                 show only keys on <iface>" << std::endl
		<< "  list-devices              list paired devices on the system" << std::endl
		<< "    [iface]                 show only devices on <iface>" << std::endl
		<< "  list-netdevs              show WPAN network device info" << std::endl
		<< "    [iface]                 show only <iface>" << std::endl
		<< "  list-params <iface>       show security parameters of <iface>" << std::endl
		<< "  list <iface>              list key, devices, security parameters and netdevs" << std::endl
		<< "  list-phys                 list WPAN phys on the system" << std::endl
		<< "  save-eeprom <iface>       save network on <iface> to EEPROM" << std::endl;
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
			return teardown();

		case C_TEARDOWN:
			return teardown(next());

		case C_SETUP:
			return setup(EEP_FILE, next(""), next(""));

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

		case C_RESYNCD: {
			std::string iface = next();
			std::string cmd = next();
			if (cmd == "run") {
				run_resyncd(iface);
			} else if (cmd == "run-fg") {
				run_resyncd_sync(iface);
			} else if (cmd == "stop") {
				kill_resyncd(iface);
			} else {
				throw no_arg();
			}
			return 0;
		}

		case C_LIST_KEYS:
			return dump_keys(next(""));

		case C_LIST_DEVICES:
			return dump_devices(next(""));

		case C_LIST_NETDEVS:
			return dump_netdevs(next(""));

		case C_LIST_PARAMS:
			return dump_params(next());

		case C_LIST: {
			std::string iface = next();
			return dump_keys(iface) || dump_devices(iface) ||
				dump_params(iface) || dump_netdevs(iface);
		}

		case C_LIST_PHYS:
			return dump_phys();

		case C_SAVE_EEPROM:
			return save_eeprom(EEP_FILE, next());

		case C_MONITOR:
			return add_monitor(next(), next(""));

		case C_INIT_EEPROM:
			return init_eeprom(EEP_FILE, next());

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
