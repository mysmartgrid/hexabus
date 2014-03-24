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
#include "controller.hpp"
#include "bootstrap.hpp"

#include <boost/lexical_cast.hpp>

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

void hexdump(const void *data, int len)
{
	using namespace std;

	const uint8_t *p = reinterpret_cast<const uint8_t*>(data);
	int offset = 0;
	stringstream ss;

	ss << hex << setfill('0');

	while (len > 0) {
		int i;

		ss << setw(8) << offset;

		for (i = 0; i < 16 && len - i > 0; i++)
			ss << " " << setw(2) << int(p[offset + i]);
		while (i++ < 16)
			ss << "   ";

		ss << " ";

		for (i = 0; i < 16 && len - i > 0; i++)
			if (isprint(p[offset + i]))
				ss << p[offset + i];
			else
				ss << ".";

		ss << std::endl;

		len -= 16;
		offset += 16;
	}

	cout << ss.str();
}

void get_random(void* target, size_t len)
{
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("open()");

	read(fd, target, len);
	close(fd);
}

std::pair<std::string, PAN> setup_random_network()
{
	Controller ctrl;
	Phy phy = ctrl.list_phys().at(0);

	uint8_t keybytes[16];
	uint16_t pan_id;

	get_random(keybytes, 16);
	get_random(&pan_id, sizeof(pan_id));

	PAN pan(pan_id, 2, 0);

	NetDevice wpan = ctrl.add_netdev(phy, htobe64(0xdeadbeefcafebabeULL));

	ctrl.start(wpan.name(), pan);
	ctrl.setup_phy(phy.name());

	ctrl.add_key(Key::indexed(wpan.name(), 1 << 1, 0, keybytes, 0));
	ctrl.enable_security(wpan.name());

	std::string sys_cmd = "ip link add link " + wpan.name() + " name "
		+ wpan.name() + ".lp0 type lowpan; ip link set " + wpan.name()
		+ " up; ip link set " + wpan.name() + ".lp0 up";

	std::cout << sys_cmd << std::endl;
	system(sys_cmd.c_str());

	return std::make_pair(wpan.name(), pan);
}

void run_pairing(const std::string& dev, uint16_t pan)
{
	PairingHandler handler(dev, pan);

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

	if (cmd == "setup-random") {
		std::pair<std::string, PAN> net = setup_random_network();

		std::cout << boost::format("Device: %1%") % net.first << std::endl
			<< boost::format("PAN: %|04x|") % net.second.pan_id() << std::endl;
	} else if (cmd == "pair") {
		if (argc < 4) {
			std::cerr << "required args: <iface> <pan-id>" << std::endl;
			return 1;
		}
		run_pairing(argv[2], strtoul(argv[3], NULL, 16));
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
