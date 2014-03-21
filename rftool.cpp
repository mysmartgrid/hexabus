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

#define HDR_PAIR "!HXB-PAIR"
#define HDR_CONN "!HXB-CONN"

class BootstrapResponder {
	private:
		int _fd;
		std::string iface;
		const PAN& pan;

		std::vector<uint8_t> packet;
		sockaddr_ieee802154 peer;

		void receive_once()
		{
			packet.resize(256);

			socklen_t peerlen = sizeof(peer);
			int len = recvfrom(_fd, &packet[0], packet.size(), 0,
					reinterpret_cast<sockaddr*>(&peer),
					&peerlen);
			if (len < 0)
				throw std::runtime_error(strerror(errno));

			packet.resize(len);
		}

		void respond_pair()
		{
			struct {
				char header[9];
				uint16_t pan_id;
				uint8_t key[16];
			} __attribute__((packed)) packet;

			memcpy(packet.header, HDR_PAIR, strlen(HDR_PAIR));
			packet.pan_id = htons(pan.pan_id());

			Controller ctrl;
			// TODO: more discriminate?
			Key key = ctrl.list_keys(iface).at(0);

			uint64_t addr;
			memcpy(&addr, peer.addr.hwaddr, 8);
			try {
				ctrl.add_device(Device(iface, pan.pan_id(), 0xfffe, addr, 0));
			} catch (const nl::nl_error& e) {
				if (e.error() != NLE_EXIST)
					throw;
			}

			// crypto off
			int val = 1;
			setsockopt(_fd, SOL_IEEE802154, 1, &val, sizeof(val));

			memcpy(packet.key, key.key(), 16);
			if (sendto(_fd, &packet, sizeof(packet), 0,
					reinterpret_cast<sockaddr*>(&peer),
					sizeof(peer)) < 0)
				throw std::runtime_error(strerror(errno));
		}

		void respond_conn()
		{
			struct {
				char header[9];
				uint32_t ctr;
			} __attribute__((packed)) packet;

			memcpy(packet.header, HDR_CONN, strlen(HDR_CONN));

			Controller ctrl;
			uint64_t addr;

			memcpy(&addr, peer.addr.hwaddr, 8);
			Device dev = ctrl.list_devices(iface, addr).at(0);

			// crypto default
			int val = 0;
			setsockopt(_fd, SOL_IEEE802154, 1, &val, sizeof(val));

			packet.ctr = htonl(dev.frame_ctr());
			if (sendto(_fd, &packet, sizeof(packet), 0,
					reinterpret_cast<sockaddr*>(&peer),
					sizeof(peer)) < 0)
				throw std::runtime_error(strerror(errno));
		}

	public:
		BootstrapResponder(const std::string& iface, const PAN& pan)
			: _fd(socket(AF_IEEE802154, SOCK_DGRAM, 0)), iface(iface),
			  pan(pan)
		{
			if (_fd < 0)
				throw std::runtime_error(strerror(errno));
		}

		virtual ~BootstrapResponder()
		{
			close(_fd);
		}

		void once()
		{
			receive_once();

			if (!memcmp(&packet[0], HDR_PAIR, strlen(HDR_PAIR))) {
				respond_pair();
			} else if (!memcmp(&packet[0], HDR_CONN, strlen(HDR_CONN))) {
				respond_conn();
			}
		}
};

void run_network(const std::string& dev, const PAN& pan)
{
	BootstrapResponder resp(dev, pan);

	while (true) {
		try {
			resp.once();
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

	if (cmd == "run") {
		std::pair<std::string, PAN> net = setup_random_network();
		run_network(net.first, net.second);
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
