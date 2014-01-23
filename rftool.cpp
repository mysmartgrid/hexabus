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

		void put(int type, const std::string& str)
		{
			int err = nla_put(msg, type, str.size() + 1,
				str.c_str());

			if (err)
				throw std::runtime_error(nl_geterror(err));
		}

		void put(int type, int32_t i)
		{
			int err = nla_put(msg, type, sizeof(i), &i);

			if (err)
				throw std::runtime_error(nl_geterror(err));
		}

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
		list_phy(int family, const char* iface = 0)
			: nlmsg(family, 0,
				NLM_F_REQUEST | (iface ? 0 : NLM_F_DUMP),
				IEEE802154_LIST_PHY)
		{
			if (iface)
				put(IEEE802154_ATTR_PHY_NAME, iface);
		}
};

class phy_setparams : public nlmsg {
	public:
		phy_setparams(int family, const char* phy)
			: nlmsg(family, 0, NLM_F_REQUEST,
				IEEE802154_SET_PHYPARAMS)
		{
			put(IEEE802154_ATTR_PHY_NAME, phy);
		}

		void txpower(int txp)
		{
			put(IEEE802154_ATTR_TXPOWER, txp);
		}
};

}



#define NLA_IEEE802154_HW_ADDR NLA_U64

namespace parsers {

class parser {
	public:
		class nlattrs {
			friend class parser;

			private:
				enum nlattr_type {
					NLA_UNSPEC,
					NLA_U8,
					NLA_U16,
					NLA_U32,
					NLA_U64,
					NLA_STRING,
					NLA_FLAG,
					NLA_MSECS,
					NLA_NESTED,
					NLA_NESTED_COMPAT,
					NLA_NUL_STRING,
					NLA_BINARY,
					NLA_S8,
					NLA_S16,
					NLA_S32,
					NLA_S64,
					__NLA_TYPE_MAX,
				};

				std::vector<struct nlattr*>& data;

				nlattrs(std::vector<struct nlattr*>& data)
					: data(data)
				{
				}

				void check_type(size_t attr, int type) const
				{
					std::cout << nla_type(data.at(attr)) << ":" << NLA_STRING << std::endl;
					if (nla_type(data.at(attr)) != type)
						throw std::logic_error("type mistmatch");
				}

			public:
				bool operator[](size_t attr) const
				{
					return data.at(attr);
				}

				const char* str(size_t attr) const
				{
					check_type(attr, NLA_STRING);
					return nla_get_string(data[attr]);
				}

				uint8_t u8(size_t attr) const
				{
					check_type(attr, NLA_U8);
					return nla_get_u8(data[attr]);
				}

				int32_t s32(size_t attr) const
				{
					check_type(attr, NLA_S32);
					return *static_cast<const int32_t*>(raw(attr));
				}

				size_t length(size_t attr) const
				{
					return nla_len(data[attr]);
				}

				const void* raw(size_t attr) const
				{
					return nla_data(data[attr]);
				}
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

//			p.name = attrs.str(IEEE802154_ATTR_PHY_NAME);
			p.channel = attrs.u8(IEEE802154_ATTR_CHANNEL);
//			p.page = attrs.u8(IEEE802154_ATTR_PAGE);
//			if (attrs[IEEE802154_ATTR_TXPOWER]) {
//				p.txpower = attrs.s32(IEEE802154_ATTR_TXPOWER);
//			}

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

		void set_txpower(int txp)
		{
			msgs::phy_setparams cmd(family, "wpan-phy0");
			parsers::parser p;

			cmd.txpower(txp);
			send(cmd);
			recv(p);
		}
};

int main(int, const char*[])
{
	nlsock sock;

	sock.set_txpower(3);
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
}
