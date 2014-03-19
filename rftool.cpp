#include <errno.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ieee802154.h"
#include "nl802154.h"

#include "netlink.hpp"
#include "types.hpp"

#include <stdexcept>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>

struct phy {
	std::string name;
	uint32_t page, channel;
	std::vector<uint32_t> pages;
	int txpower, cca_mode, ed_level;
	bool lbt, aack, retransmit;
};

struct key {
	std::string iface;
	int mode, id, frame_types;
	uint32_t cmd_frame_ids;
	uint8_t source[8];
	int source_len;
	uint8_t key[16];
};

struct seclevel {
	std::string iface;
	uint8_t frame_type, levels;
};

namespace msgs {

struct list_phy : nlmsg {
	list_phy(int family, const std::string& iface = "")
		: nlmsg(family, 0,
			NLM_F_REQUEST | (iface.size() ? 0 : NLM_F_DUMP),
			IEEE802154_LIST_PHY)
	{
		if (iface.size())
			put(IEEE802154_ATTR_PHY_NAME, iface);
	}
};

struct add_iface : nlmsg {
	add_iface(int family, const std::string& phy, const std::string& iface = "")
		: nlmsg(family, 0, NLM_F_REQUEST, IEEE802154_ADD_IFACE)
	{
		put(IEEE802154_ATTR_PHY_NAME, phy);
		if (iface.size())
			put(IEEE802154_ATTR_DEV_NAME, iface);

		put<uint8_t>(IEEE802154_ATTR_DEV_TYPE, IEEE802154_DEV_WPAN);
	}

	void hwaddr(const uint8_t addr[8])
	{ put_raw(IEEE802154_ATTR_HW_ADDR, addr, IEEE802154_ADDR_LEN); }
};

struct start : nlmsg {
	start(int family, const std::string& dev)
		: nlmsg(family, 0, NLM_F_REQUEST, IEEE802154_START_REQ)
	{
		put<uint8_t>(IEEE802154_ATTR_BCN_ORD, 15);
		put<uint8_t>(IEEE802154_ATTR_SF_ORD, 15);
		put<uint8_t>(IEEE802154_ATTR_BAT_EXT, 0);
		put<uint8_t>(IEEE802154_ATTR_COORD_REALIGN, 0);
		put<uint8_t>(IEEE802154_ATTR_PAN_COORD, 0);
		put(IEEE802154_ATTR_DEV_NAME, dev);
	}

	void pan_id(uint16_t pan)
	{ put(IEEE802154_ATTR_COORD_PAN_ID, pan); }

	void short_addr(uint16_t addr)
	{ put(IEEE802154_ATTR_COORD_SHORT_ADDR, addr); }

	void channel(uint8_t channel)
	{ put(IEEE802154_ATTR_CHANNEL, channel); }

	void page(uint8_t page)
	{ put(IEEE802154_ATTR_PAGE, page); }
};

struct phy_setparams : nlmsg {
	phy_setparams(int family, const std::string& phy)
		: nlmsg(family, 0, NLM_F_REQUEST,
			IEEE802154_SET_PHYPARAMS)
	{
		put(IEEE802154_ATTR_PHY_NAME, phy);
	}

	void txpower(int32_t txp)
	{ put(IEEE802154_ATTR_TXPOWER, txp); }

	void lbt(bool enable)
	{ put<uint8_t>(IEEE802154_ATTR_LBT_ENABLED, enable); }

	void cca_mode(uint8_t mode)
	{ put(IEEE802154_ATTR_CCA_MODE, mode); }

	void ed_level(int32_t level)
	{ put(IEEE802154_ATTR_CCA_ED_LEVEL, level); }

	void frame_retries(int8_t retries)
	{ put(IEEE802154_ATTR_FRAME_RETRIES, retries); }
};

struct list_keys : nlmsg {
	list_keys(int family, const std::string& iface = "")
		: nlmsg(family, 0,
			NLM_F_REQUEST | (iface.size() ? 0 : NLM_F_DUMP),
			IEEE802154_LLSEC_LIST_KEY)
	{
		if (iface.size())
			put(IEEE802154_ATTR_DEV_NAME, iface);
	}
};

struct modify_key : nlmsg {
	modify_key(int family, int msg, const std::string iface)
		: nlmsg(family, 0, NLM_F_REQUEST, msg)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}

	void frames(uint8_t frames)
	{ put(IEEE802154_ATTR_LLSEC_KEY_USAGE_FRAME_TYPES, frames); }

	void id(uint8_t id)
	{ put(IEEE802154_ATTR_LLSEC_KEY_ID, id); }

	void mode(uint8_t mode)
	{ put(IEEE802154_ATTR_LLSEC_KEY_MODE, mode); }
};

struct add_key : modify_key {
	add_key(int family, const std::string iface)
		: modify_key(family, IEEE802154_LLSEC_ADD_KEY, iface)
	{}

	void key(const void *key)
	{ put_raw(IEEE802154_ATTR_LLSEC_KEY_BYTES, key, 16); }
};

struct del_key : modify_key {
	del_key(int family, const std::string iface)
		: modify_key(family, IEEE802154_LLSEC_DEL_KEY, iface)
	{}
};

struct list_devs : nlmsg {
	list_devs(int family, const std::string& iface = "")
		: nlmsg(family, 0,
			NLM_F_REQUEST | (iface.size() ? 0 : NLM_F_DUMP),
			IEEE802154_LLSEC_LIST_DEV)
	{
		if (iface.size())
			put(IEEE802154_ATTR_DEV_NAME, iface);
	}
};

struct modify_dev : nlmsg {
	modify_dev(int family, int msg, const std::string& iface)
		: nlmsg(family, 0, NLM_F_REQUEST, msg)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}

	void pan_id(uint16_t id)
	{ put(IEEE802154_ATTR_PAN_ID, id); }

	void short_addr(uint16_t addr)
	{ put(IEEE802154_ATTR_SHORT_ADDR, addr); }

	void hwaddr(const uint8_t addr[8])
	{ put_raw(IEEE802154_ATTR_HW_ADDR, addr, IEEE802154_ADDR_LEN); }

	void frame_ctr(uint32_t ctr)
	{ put(IEEE802154_ATTR_LLSEC_FRAME_COUNTER, ctr); }
};

struct add_dev : modify_dev {
	add_dev(int family, const std::string& iface)
		: modify_dev(family, IEEE802154_LLSEC_ADD_DEV, iface)
	{}
};

struct del_dev : modify_dev {
	del_dev(int family, const std::string& iface)
		: modify_dev(family, IEEE802154_LLSEC_DEL_DEV, iface)
	{}
};

struct list_seclevels : nlmsg {
	list_seclevels(int family, const std::string& iface = "")
		: nlmsg(family, 0,
			NLM_F_REQUEST | (iface.size() ? 0 : NLM_F_DUMP),
			IEEE802154_LLSEC_LIST_SECLEVEL)
	{
		if (iface.size())
			put(IEEE802154_ATTR_DEV_NAME, iface);
	}
};

struct modify_seclevel : nlmsg {
	modify_seclevel(int family, int msg, const std::string& iface)
		: nlmsg(family, 0, NLM_F_REQUEST, msg)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}

	void frame(uint8_t id)
	{ put(IEEE802154_ATTR_LLSEC_FRAME_TYPE, id); }

	void levels(uint8_t levels)
	{ put(IEEE802154_ATTR_LLSEC_SECLEVELS, levels); }
};

struct add_seclevel : modify_seclevel {
	add_seclevel(int family, const std::string& iface)
		: modify_seclevel(family, IEEE802154_LLSEC_ADD_SECLEVEL, iface)
	{}
};

struct del_seclevel : modify_seclevel {
	del_seclevel(int family, const std::string& iface)
		: modify_seclevel(family, IEEE802154_LLSEC_DEL_SECLEVEL, iface)
	{}
};

struct llsec_setparams : nlmsg {
	llsec_setparams(int family, const std::string& iface)
		: nlmsg(family, 0, NLM_F_REQUEST, IEEE802154_LLSEC_SETPARAMS)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}

	void enabled(bool on)
	{ put(IEEE802154_ATTR_LLSEC_ENABLED, on); }

	void out_level(uint8_t level)
	{ put(IEEE802154_ATTR_LLSEC_SECLEVEL, level); }

	void key_id(uint8_t id)
	{ put(IEEE802154_ATTR_LLSEC_KEY_ID, id); }

	void key_mode(uint8_t mode)
	{ put(IEEE802154_ATTR_LLSEC_KEY_MODE, mode); }

	void key_source_hw(uint64_t hw)
	{ put(IEEE802154_ATTR_HW_ADDR, hw); }
};

}



#define NLA_IEEE802154_HW_ADDR NLA_U64

namespace parsers {

class list_phy : public parser {
	private:
		::msgs::list_phy& cmd;
		std::vector<phy>& result;

	public:
		list_phy(::msgs::list_phy& cmd, std::vector<phy>& result)
			: cmd(cmd), result(result)
		{
			has_valid();
		}

		virtual int valid(const nlattrs& attrs)
		{
			phy p;

			if (!attrs[IEEE802154_ATTR_PHY_NAME] ||
				!attrs[IEEE802154_ATTR_CHANNEL] ||
				!attrs[IEEE802154_ATTR_PAGE]) {
				std::cout << "abort" << std::endl;
				return NL_STOP;
			}

			p.name = attrs.str(IEEE802154_ATTR_PHY_NAME);
			p.channel = attrs.u8(IEEE802154_ATTR_CHANNEL);
			p.page = attrs.u8(IEEE802154_ATTR_PAGE);
			if (attrs[IEEE802154_ATTR_TXPOWER])
				p.txpower = attrs.s8(IEEE802154_ATTR_TXPOWER);
			if (attrs[IEEE802154_ATTR_LBT_ENABLED])
				p.lbt = attrs.u8(IEEE802154_ATTR_LBT_ENABLED);
			if (attrs[IEEE802154_ATTR_CCA_MODE])
				p.cca_mode = attrs.u8(IEEE802154_ATTR_CCA_MODE);
			if (attrs[IEEE802154_ATTR_CCA_ED_LEVEL])
				p.ed_level = attrs.s32(IEEE802154_ATTR_CCA_ED_LEVEL);


			if (attrs[IEEE802154_ATTR_CHANNEL_PAGE_LIST]) {
				int len = attrs.length(IEEE802154_ATTR_CHANNEL_PAGE_LIST);
				if (len % 4 != 0) {
					return NL_STOP;
				}

				const uint32_t *data = static_cast<const uint32_t*>(attrs.raw(IEEE802154_ATTR_CHANNEL_PAGE_LIST));

				for (int i = 0; i < len / 4; i++) {
					while (p.pages.size() <= data[i] >> 27)
						p.pages.push_back(0);
					p.pages[data[i] >> 27] = data[i] & 0x07FFFFFF;
				}
			}

			result.push_back(p);

			return NL_OK;
		}
};

class add_iface : public parser {
	private:
		std::string _name;
		std::string _phy;

	public:
		add_iface()
		{
			has_valid();
		}

		virtual int valid(const nlattrs& attrs)
		{
			_name = attrs.str(IEEE802154_ATTR_DEV_NAME);
			_phy = attrs.str(IEEE802154_ATTR_PHY_NAME);
			return NL_OK;
		}

		const std::string& name() const { return _name; }
		const std::string& phy() const { return _phy; }
};

class list_keys : public parser {
	private:
		std::vector<key>& _keys;

	public:
		list_keys(std::vector<key>& keys)
			: _keys(keys)
		{
			has_valid();
		}

		virtual int valid(const nlattrs& attrs)
		{
			key key;

			key.iface = attrs.str(IEEE802154_ATTR_DEV_NAME);
			key.mode = attrs.u8(IEEE802154_ATTR_LLSEC_KEY_MODE);
			memcpy(key.key, attrs.raw(IEEE802154_ATTR_LLSEC_KEY_BYTES), 16);
			key.frame_types = attrs.u8(IEEE802154_ATTR_LLSEC_KEY_USAGE_FRAME_TYPES);
			key.cmd_frame_ids = 0;
			if (key.mode != 0) {
				key.id = attrs.u8(IEEE802154_ATTR_LLSEC_KEY_ID);
			}
			key.source_len = 0;
			if (attrs[IEEE802154_ATTR_LLSEC_KEY_SOURCE_SHORT]) {
				key.source_len = 4;
			} else if (attrs[IEEE802154_ATTR_LLSEC_KEY_SOURCE_EXTENDED]) {
				key.source_len = 8;
			}
			memcpy(key.source, attrs.raw(IEEE802154_ATTR_LLSEC_KEY_SOURCE_EXTENDED), key.source_len);

			_keys.push_back(key);

			return NL_OK;
		}
};

class list_devs : public parser {
	private:
		std::vector<Device>& _devs;

	public:
		list_devs(std::vector<Device>& devs)
			: _devs(devs)
		{
			has_valid();
		}

		virtual int valid(const nlattrs& attrs)
		{
			std::string iface = attrs.str(IEEE802154_ATTR_DEV_NAME);
			uint16_t pan_id = attrs.u16(IEEE802154_ATTR_PAN_ID);
			uint16_t short_addr = attrs.u16(IEEE802154_ATTR_SHORT_ADDR);
			uint64_t hwaddr = attrs.u64(IEEE802154_ATTR_HW_ADDR);
			uint32_t frame_ctr = attrs.u32(IEEE802154_ATTR_LLSEC_FRAME_COUNTER);

			_devs.push_back(Device(iface, pan_id, short_addr, hwaddr, frame_ctr));

			return NL_OK;
		}
};

class list_seclevels : public parser {
	private:
		std::vector<seclevel>& _levels;

	public:
		list_seclevels(std::vector<seclevel>& levels)
			: _levels(levels)
		{
			has_valid();
		}

		virtual int valid(const nlattrs& attrs)
		{
			seclevel sl;

			sl.iface = attrs.str(IEEE802154_ATTR_DEV_NAME);
			sl.frame_type = attrs.u8(IEEE802154_ATTR_LLSEC_FRAME_TYPE);
			sl.levels = attrs.u8(IEEE802154_ATTR_LLSEC_SECLEVELS);

			_levels.push_back(sl);

			return NL_OK;
		}
};

}



class nlsock {
	private:
		struct nl_sock* nl;
		int family;

		int send(msgs::nlmsg& msg)
		{
			int res = nl_send_auto(nl, msg.raw());
			if (res < 0)
				throw std::runtime_error(nl_geterror(res));

			return res;
		}

		int recv(parsers::parser& parser)
		{
			int res = nl_recvmsgs(nl, parser.raw());
			if (res < 0) {
				std::cout << "err " << res << std::endl;
				throw std::runtime_error(nl_geterror(res));
			}

			return res;
		}

		nlsock& operator=(const nlsock&);
		nlsock(const nlsock&);

	public:
		nlsock()
			: nl(nl_socket_alloc())
		{
			if (!nl)
				throw std::bad_alloc();

			int err = genl_connect(nl);
			if (err < 0)
				throw std::runtime_error(nl_geterror(err));

			family = genl_ctrl_resolve(nl, IEEE802154_NL_NAME);
			if (family < 0)
				throw std::runtime_error(nl_geterror(family));
		}

		~nlsock()
		{
			nl_socket_free(nl);
		}

		std::vector<phy> list_phy()
		{
			msgs::list_phy cmd(family);
			std::vector<phy> result;
			parsers::list_phy parser(cmd, result);

			send(cmd);
			recv(parser);

			return result;
		}

		std::vector<key> list_keys(const std::string& dev = "")
		{
			msgs::list_keys cmd(family, dev);
			std::vector<key> keys;
			parsers::list_keys parser(keys);

			send(cmd);
			recv(parser);

			return keys;
		}

		void set_lbt(const std::string& dev, bool lbt)
		{
			msgs::phy_setparams cmd(family, dev);
			parsers::parser p;

			cmd.lbt(lbt);
			send(cmd);
			recv(p);
		}

		void set_txpower(const std::string& dev, int txp)
		{
			msgs::phy_setparams cmd(family, dev);
			parsers::parser p;

			cmd.txpower(txp);
			send(cmd);
			recv(p);
		}

		void set_cca_mode(const std::string& dev, uint8_t mode)
		{
			msgs::phy_setparams cmd(family, dev);
			parsers::parser p;

			cmd.cca_mode(mode);
			send(cmd);
			recv(p);
		}

		void set_ed_level(const std::string& dev, int32_t level)
		{
			msgs::phy_setparams cmd(family, dev);
			parsers::parser p;

			cmd.ed_level(level);
			send(cmd);
			recv(p);
		}

		void set_frame_retries(const std::string& dev, int8_t retries)
		{
			msgs::phy_setparams cmd(family, dev);
			parsers::parser p;

			cmd.frame_retries(retries);
			send(cmd);
			recv(p);
		}

		void add_iface(const std::string& phy, const std::string& iface = "")
		{
			msgs::add_iface cmd(family, phy, iface);
			parsers::parser p;
			parsers::add_iface pi;

			uint8_t addr[8] = { 0xde, 0xad, 0xbe, 0xef,
				0xca, 0xfe, 0xba, 0xbe };

			cmd.hwaddr(addr);
			send(cmd);
			recv(pi);
			recv(p);

			std::cout << "added " << pi.name() << " to " << pi.phy() << std::endl;
		}

		void start(const std::string& dev)
		{
			msgs::start start(family, dev);
			parsers::parser p;

			start.short_addr(0xfffe);
			start.pan_id(0x0900);
			start.channel(0);
			start.page(0);
			start.page(2);

			send(start);
			recv(p);
		}

		void add_def_key()
		{
			msgs::add_key akey(family, "wpan8");
			parsers::parser p;
			uint8_t keybytes[16] = { 0x01, 0x4f, 0x30, 0x92, 0xdf,
				0x53, 0xd4, 0x70, 0x17, 0x29, 0xde, 0x1d, 0x31,
				0xbb, 0x55, 0x45 };

			akey.frames(1 << 1);
			akey.id(0);
			akey.mode(1);
			akey.key(keybytes);

			send(akey);
			recv(p);
		}

		void add_dev_801c(const std::string& iface)
		{
			msgs::add_dev cmd(family, iface);
			parsers::parser p;
			uint8_t addr[8] = { 0x02, 0x50, 0xc4, 0xff, 0xfe,
				0x04, 0x81, 0x0c };

			cmd.pan_id(0x0900);
			cmd.short_addr(0xfffe);
			cmd.hwaddr(addr);
			cmd.frame_ctr(0);

			send(cmd);
			recv(p);
		}

		std::vector<Device> list_devs(const std::string& iface = "")
		{
			msgs::list_devs cmd(family, iface);
			std::vector<Device> result;
			parsers::list_devs p(result);

			send(cmd);
			recv(p);

			return result;
		}

		void add_def_seclevel()
		{
			msgs::add_seclevel msg(family, "wpan8");
			parsers::parser p;

			msg.frame(1);
			msg.levels(0xff);

			send(msg);
			recv(p);
		}

		void set_secen()
		{
			msgs::llsec_setparams msg(family, "wpan8");
			parsers::parser p;

			msg.enabled(true);
			msg.out_level(5);
			msg.key_id(0);
			msg.key_mode(1);
			msg.key_source_hw(0xffffffffffffffffULL);

			send(msg);
			recv(p);
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
			std::vector<key> keys = sock.list_keys("wpan8");

			for (size_t i = 0; i < keys.size(); i++) {
				key& k = keys[i];

				std::cout << "key " << i << " on " << k.iface << std::endl
					<< "	mode " << k.mode << std::endl
					<< "	raw ";
				hexdump(k.key, 16);
				std::cout << std::endl;
				if (k.mode == 0)
					continue;

				std::cout << "	id " << k.id << std::endl;
				if (k.mode == 1)
					continue;

				std::cout << "	source ";
				hexdump(k.source, k.source_len);
				std::cout << std::endl;
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
				if (len == strlen(prov_hdr) &&
					!memcmp(buffer, prov_hdr, len)) {
					std::cout << "Got request" << std::endl;
					hexdump(&peer, sizeof(peer));
					sendto(sock, reply, sizeof(reply), 0, (sockaddr*) &peer, sizeof(peer));
				} else if (len == strlen(prov_recon) &&
					!memcmp(buffer, prov_recon, len)) {
					std::cout << "Got resync" << std::endl;
					sendto(sock, reply_con, sizeof(reply_con), 0, (sockaddr*) &peer, sizeof(peer));
				}
			}
		}

		return 0;
	}

	phyname = sock.list_phy().at(0).name;

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

	std::vector<phy> phys = sock.list_phy();
	for (size_t i = 0; i < phys.size(); i++) {
		phy& p = phys[i];

		std::cout
			<< p.name << std::endl
			<< "	channel " << p.channel << std::endl
			<< "	page " << p.page << std::endl
			<< "	txpower " << p.txpower << std::endl
			<< "	lbt? " << p.lbt << std::endl
			<< "	cca mode " << p.cca_mode << std::endl
			<< "	cca ed level " << p.ed_level << std::endl;

		for (size_t j = 0; j < p.pages.size(); j++) {
			if (p.pages[j])
				std::cout
					<< "	page " << j
					<< ": " << p.pages[j] << std::endl;
		}
	}
}
