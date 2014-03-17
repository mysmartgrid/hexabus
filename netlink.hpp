#ifndef NETLINK_HPP_
#define NETLINK_HPP_

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>

#include <vector>
#include <string>
#include <stdexcept>

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

}

namespace parsers {

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

		int8_t s8(size_t attr) const
		{ return check_fixed<int8_t>(attr); }

		uint8_t u8(size_t attr) const
		{ return check_fixed<uint8_t>(attr); }

		uint16_t u16(size_t attr) const
		{ return check_fixed<uint16_t>(attr); }

		uint32_t u32(size_t attr) const
		{ return check_fixed<uint32_t>(attr); }

		int32_t s32(size_t attr) const
		{ return check_fixed<int32_t>(attr); }

		size_t length(size_t attr) const
		{ return nla_len(data[attr]); }

		const void* raw(size_t attr) const
		{ return nla_data(data[attr]); }
};

class parser {
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
			: cb(nl_cb_alloc(NL_CB_DEFAULT))
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

}

#endif
