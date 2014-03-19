#ifndef NETLINK_HPP_
#define NETLINK_HPP_

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>

#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>

namespace nl {

class msg {
	private:
		msg& operator=(const msg&);
		msg(const msg&);

	protected:
		struct nl_msg* _msg;

		void put_raw(int type, const void* data, size_t len);

		void put(int type, const std::string& str)
		{ put_raw(type, str.c_str(), str.size() + 1); }

		template<typename T>
		void put(int type, T i)
		{ put_raw(type, &i, sizeof(i)); }

	public:
		msg(int family, int hdrlen, int flags, int cmd);

		virtual ~msg();

		 struct nl_msg* raw() { return _msg; }
};

class attrs {
	private:
		const std::vector<struct nlattr*>& data;

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
		attrs(const std::vector<struct nlattr*>& data)
			: data(data)
		{}

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

		uint64_t u64(size_t attr) const
		{ return check_fixed<uint64_t>(attr); }

		int32_t s32(size_t attr) const
		{ return check_fixed<int32_t>(attr); }

		size_t length(size_t attr) const
		{ return nla_len(data[attr]); }

		const void* raw(size_t attr) const
		{ return nla_data(data[attr]); }
};

template<typename Result, int MaxAttr>
class parser {
	protected:
		struct nl_cb *cb;

		parser(const parser&);
		parser& operator=(const parser&);

		static int cb_valid(struct nl_msg* msg, void* arg)
		{
			struct nlmsghdr* nlh = nlmsg_hdr(msg);
			std::vector<struct nlattr*> nlattrs(MaxAttr + 1);

			genlmsg_parse(nlh, 0, &nlattrs[0], MaxAttr, 0);

			attrs attrs(nlattrs);

			try {
				return static_cast<parser*>(arg)->valid(attrs);
			} catch (...) {
				std::cerr << "nl::parser::valid() threw" << std::endl;
				return NL_STOP;
			}
		}

	public:
		parser()
			: cb(nl_cb_alloc(NL_CB_DEFAULT))
		{
			if (!cb)
				throw std::bad_alloc();

			nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, &cb_valid, this);
		}

		virtual ~parser()
		{
			nl_cb_put(cb);
		}

		struct nl_cb* raw() { return cb; }

		virtual int valid(const attrs& attrs)
		{ return NL_OK; }

		virtual Result complete() = 0;
};

class socket {
	private:
		struct nl_sock* nl;

		socket& operator=(const socket&);
		socket(const socket&);

	public:
		socket(int family);

		virtual ~socket();

		int send(msg& msg);

		template<typename Result, int MaxAttr>
		Result recv(nl::parser<Result, MaxAttr>& parser)
		{
			int res = nl_recvmsgs(nl, parser.raw());
			if (res < 0) {
				throw std::runtime_error(nl_geterror(res));
			}

			return parser.complete();
		}
};

int get_family(const char* family_name);

}

#endif
