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

#include "ieee802154.h"
#include "nl802154.h"

#include "netlink.hpp"
#include "nlmessages.hpp"
#include "nlparsers.hpp"
#include "types.hpp"

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

class nlsock : public nl::socket {
	protected:
		void recv_errno()
		{
			parsers::errno_parser p;

			recv(p);
		}

	public:
		nlsock()
			: socket(msgs::msg802154::family())
		{}

		std::vector<Phy> list_phy()
		{
			msgs::list_phy cmd;
			parsers::list_phy parser;

			send(cmd);
			return recv(parser);
		}

		std::vector<Key> list_keys(const std::string& dev = "")
		{
			msgs::list_keys cmd(dev);
			parsers::list_keys parser;

			send(cmd);
			return recv(parser);
		}

		void set_lbt(const std::string& dev, bool lbt)
		{
			msgs::phy_setparams cmd(dev);

			cmd.lbt(lbt);
			send(cmd);
			recv_errno();
		}

		void set_txpower(const std::string& dev, int txp)
		{
			msgs::phy_setparams cmd(dev);

			cmd.txpower(txp);
			send(cmd);
			recv_errno();
		}

		void set_cca_mode(const std::string& dev, uint8_t mode)
		{
			msgs::phy_setparams cmd(dev);

			cmd.cca_mode(mode);
			send(cmd);
			recv_errno();
		}

		void set_ed_level(const std::string& dev, int32_t level)
		{
			msgs::phy_setparams cmd(dev);

			cmd.ed_level(level);
			send(cmd);
			recv_errno();
		}

		void set_frame_retries(const std::string& dev, int8_t retries)
		{
			msgs::phy_setparams cmd(dev);

			cmd.frame_retries(retries);
			send(cmd);
			recv_errno();
		}

		void add_iface(const std::string& phy, const std::string& iface = "")
		{
			msgs::add_iface cmd(phy, iface);
			parsers::add_iface pi;

			cmd.hwaddr(htobe64(0xdeadbeefcafebabeULL));
			send(cmd);
			NetDevice dev = recv(pi);
			recv_errno();

			std::cout << "added " << dev.name() << " to " << dev.phy() << std::endl;
		}

		void start(const std::string& dev)
		{
			msgs::start start(dev);

			start.short_addr(0xfffe);
			start.pan_id(0x0900);
			start.channel(0);
			start.page(0);
			start.page(2);

			send(start);
			recv_errno();
		}

		void add_def_key()
		{
			msgs::add_key akey("wpan8");
			uint8_t keybytes[16] = { 0x01, 0x4f, 0x30, 0x92, 0xdf,
				0x53, 0xd4, 0x70, 0x17, 0x29, 0xde, 0x1d, 0x31,
				0xbb, 0x55, 0x45 };

			akey.frames(1 << 1);
			akey.id(0);
			akey.mode(1);
			akey.key(keybytes);

			send(akey);
			recv_errno();
		}

		void add_dev_801c(const std::string& iface)
		{
			msgs::add_dev cmd(iface);

			cmd.pan_id(0x0900);
			cmd.short_addr(0xfffe);
			cmd.hwaddr(htobe64(0x0250c4fffe04810cULL));
			cmd.frame_ctr(0);

			send(cmd);
			recv_errno();
		}

		std::vector<Device> list_devs(const std::string& iface = "")
		{
			msgs::list_devs cmd(iface);
			parsers::list_devs p;

			send(cmd);
			return recv(p);
		}

		void add_def_seclevel()
		{
			msgs::add_seclevel msg("wpan8");

			msg.frame(1);
			msg.levels(0xff);

			send(msg);
			recv_errno();
		}

		void set_secen()
		{
			msgs::llsec_setparams msg("wpan8");

			msg.enabled(true);
			msg.out_level(5);
			msg.key_id(0);
			msg.key_mode(1);
			msg.key_source_hw(std::numeric_limits<uint64_t>::max());

			send(msg);
			recv_errno();
		}
};

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

int main(int argc, const char* argv[])
{
	nlsock sock;
	std::string phyname;

	if (argc != 1) {
		if (argv[1] == std::string("keys")) {
			std::vector<Key> keys = sock.list_keys("wpan8");

			for (size_t i = 0; i < keys.size(); i++) {
				std::cout << keys[i] << std::endl;
			}
		} else if (argv[1] == std::string("devs")) {
			std::vector<Device> devs = sock.list_devs();

			for (size_t i = 0; i < devs.size(); i++) {
				Device& dev = devs[i];

				std::cout << dev << std::endl;
			}
		} else if (argv[1] == std::string("pair")) {
			int sock = socket(AF_IEEE802154, SOCK_DGRAM, 0);
			uint8_t buffer[256];
			int len, count = 0;
			char prov_hdr[] = "!HXB-PAIR";
			char prov_recon[] = "!HXB-CONN";
			char reply[] = {
				'!', 'H', 'X', 'B', '-', 'P', 'A', 'I', 'R',
				0x00, 0x09,
				0x01, 0x4f, 0x30, 0x92, 0xdf, 0x53, 0xd4, 0x70,
				0x17, 0x29, 0xde, 0x1d, 0x31, 0xbb, 0x55, 0x45 };
			char reply_con[] = {
				'!', 'H', 'X', 'B', '-', 'C', 'O', 'N', 'N',
				0x00, 0x00, 0x01, 0x00};
			struct sockaddr_ieee802154 peer;
			socklen_t peerlen = sizeof(peer);

			// crypto off
			int val = 1;
			setsockopt(sock, SOL_IEEE802154, 1, &val, sizeof(int));

			std::cout << "Listening" << std::endl;
			while ((len = recvfrom(sock, buffer, sizeof(buffer), 0, (sockaddr*) &peer, &peerlen)) > 0) {
				std::cout << "Packet " << ++count << " (" << peerlen << "):" << std::endl;
				hexdump(buffer, len);
				std::cout << std::endl;
				if (len == (int) strlen(prov_hdr) &&
					!memcmp(buffer, prov_hdr, len)) {
					std::cout << "Got request" << std::endl;
					hexdump(&peer, sizeof(peer));
					sendto(sock, reply, sizeof(reply), 0, (sockaddr*) &peer, sizeof(peer));
				} else if (len == (int) strlen(prov_recon) &&
					!memcmp(buffer, prov_recon, len)) {
					std::cout << "Got resync" << std::endl;
					sendto(sock, reply_con, sizeof(reply_con), 0, (sockaddr*) &peer, sizeof(peer));
				}
			}
		}

		return 0;
	}

	phyname = sock.list_phy().at(0).name();

	sock.set_lbt(phyname, true);
	sock.add_iface(phyname, "wpan8");
	sock.start("wpan8");
	sock.set_cca_mode(phyname, 0);
	sock.set_ed_level(phyname, -81);
	sock.set_txpower(phyname, 3);
	sock.set_frame_retries(phyname, 3);
	sock.add_def_key();
	sock.add_dev_801c("wpan8");
	sock.add_def_seclevel();
	sock.set_secen();

	std::vector<Phy> phys = sock.list_phy();
	for (size_t i = 0; i < phys.size(); i++) {
		std::cout << phys[i] << std::endl;
	}
}
