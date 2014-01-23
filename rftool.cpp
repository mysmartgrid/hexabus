#include <errno.h>
#include <netlink/errno.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "ieee802154.h"
#include "nl802154.h"

#include <stdexcept>
#include <vector>
#include <string>
#include <iostream>

struct phy {
	std::string name;
	uint32_t page, channel;
	std::vector<uint32_t> pages;
	int txpower;
};

namespace msgs {

class nlmsg {
	private:
		nlmsg& operator=(const nlmsg&);
		nlmsg(const nlmsg&);

	protected:
		struct nl_msg* msg;

		void put_raw(int type, const void* data, size_t len)
		{
			int err = nla_put(msg, type, len, data);

			if (err)
				throw std::runtime_error(nl_geterror(err));
		}

		void put(int type, const std::string& str)
		{ put_raw(type, str.c_str(), str.size() + 1); }

		template<typename T>
		void put(int type, T i)
		{ put_raw(type, &i, sizeof(i)); }

	public:
		nlmsg(int family, int hdrlen, int flags, int cmd)
			: msg(nlmsg_alloc())
		{
			if (!msg)
				throw std::bad_alloc();

			if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family,
				hdrlen, flags, cmd, 1))
				throw std::runtime_error("nlmsg() genlmsg_put");
		}

		virtual ~nlmsg()
		{
			nlmsg_free(msg);
		}

		 struct nl_msg* raw() { return msg; }
};

class list_phy : public nlmsg {
	public:
		list_phy(int family, const std::string& iface = "")
			: nlmsg(family, 0,
				NLM_F_REQUEST | (iface.size() ? 0 : NLM_F_DUMP),
				IEEE802154_LIST_PHY)
		{
			if (iface.size())
				put(IEEE802154_ATTR_PHY_NAME, iface);
		}
};

class add_iface : public nlmsg {
	public:
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

class start : public nlmsg {
	public:
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

class phy_setparams : public nlmsg {
	public:
		phy_setparams(int family, const std::string& phy)
			: nlmsg(family, 0, NLM_F_REQUEST,
				IEEE802154_SET_PHYPARAMS)
		{
			put(IEEE802154_ATTR_PHY_NAME, phy);
		}

		void txpower(int32_t txp)
		{ put(IEEE802154_ATTR_TXPOWER, txp); }
};

}



#define NLA_IEEE802154_HW_ADDR NLA_U64

namespace parsers {

class parser {
	public:
		class nlattrs {
			friend class parser;

			private:
				std::vector<struct nlattr*>& data;

				nlattrs(std::vector<struct nlattr*>& data)
					: data(data)
				{
				}

				template<typename T>
				T check_fixed(size_t attr) const
				{
					if (nla_len(data.at(attr)) != sizeof(T))
						throw std::logic_error("type mistmatch");
					return *static_cast<const T*>(nla_data(data.at(attr)));
				}

				std::string check_string(size_t attr) const
				{
					const char* p = static_cast<const char*>(nla_data(data.at(attr)));
					if (p[nla_len(data.at(attr)) - 1] != 0)
						throw std::logic_error("type mismatch");
					return p;
				}

			public:
				bool operator[](size_t attr) const
				{
					return attr > 0
						&& attr < data.size()
						&& data[attr];
				}

				std::string str(size_t attr) const
				{ return check_string(attr); }

				uint8_t u8(size_t attr) const
				{ return check_fixed<uint8_t>(attr); }

				int32_t s32(size_t attr) const
				{ return check_fixed<int32_t>(attr); }

				size_t length(size_t attr) const
				{ return nla_len(data[attr]); }

				const void* raw(size_t attr) const
				{ return nla_data(data[attr]); }
		};

	protected:
		struct nl_cb *cb;

		parser(const parser&);
		parser& operator=(const parser&);

		static int cb_valid(struct nl_msg* msg, void* arg)
		{
			struct nlmsghdr* nlh = nlmsg_hdr(msg);
			std::vector<struct nlattr*> attrs(IEEE802154_ATTR_MAX + 1);

			genlmsg_parse(nlh, 0, &attrs[0], IEEE802154_ATTR_MAX, 0);

			nlattrs nlattrs(attrs);

			return static_cast<parser*>(arg)->valid(attrs);
		}

		void has_valid()
		{
			nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, &cb_valid, this);
		}

	public:
		parser()
			: cb(nl_cb_alloc(NL_CB_DEBUG))
		{
			if (!cb)
				throw std::bad_alloc();
		}

		virtual ~parser()
		{
			nl_cb_put(cb);
		}

		struct nl_cb* raw() { return cb; }

		virtual int valid(const nlattrs& attrs)
		{ return NL_OK; }
};

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

		virtual int valid(const parser::nlattrs& attrs)
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
			if (attrs[IEEE802154_ATTR_TXPOWER]) {
				p.txpower = attrs.s32(IEEE802154_ATTR_TXPOWER);
			}

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

		virtual int valid(const parser::nlattrs& attrs)
		{
			_name = attrs.str(IEEE802154_ATTR_DEV_NAME);
			_phy = attrs.str(IEEE802154_ATTR_PHY_NAME);
			return NL_OK;
		}

		const std::string& name() const { return _name; }
		const std::string& phy() const { return _phy; }
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
			if (res < 0)
				throw std::runtime_error(nl_geterror(res));

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

		void set_txpower(const std::string& dev, int txp)
		{
			msgs::phy_setparams cmd(family, dev);
			parsers::parser p;

			cmd.txpower(txp);
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

			start.short_addr(0x1234);
			start.pan_id(0x031f);
			start.channel(0);
			start.page(2);

			send(start);
		}
};

int main(int, const char*[])
{
	nlsock sock;

	std::vector<phy> phys = sock.list_phy();
	for (size_t i = 0; i < phys.size(); i++) {
		phy& p = phys[i];

		std::cout
			<< p.name << std::endl
			<< "	channel " << p.channel << std::endl
			<< "	page " << p.page << std::endl
			<< "	txpower " << p.txpower << std::endl;

		for (size_t j = 0; j < p.pages.size(); j++) {
			std::cout
				<< "	page " << j
					<< ": " << p.pages[j] << std::endl;
		}
	}
	sock.set_txpower("wpan-phy0", 3);
	sock.add_iface("wpan-phy0", "wpan8");
	sock.start("wpan8");
}
