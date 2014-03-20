#ifndef NLPARSERS_HPP_
#define NLPARSERS_HPP_

#include <string>

#include "netlink.hpp"
#include "nl802154.h"
#include "types.hpp"

namespace parsers {

template<class Result>
class base_parser : public nl::parser<Result, IEEE802154_ATTR_MAX> {
};

template<typename Member>
class list_parser : public base_parser<const std::vector<Member>&> {
	protected:
		std::vector<Member> list;

	public:
		const std::vector<Member>& complete() { return list; }
};

struct list_phy : list_parser<Phy> {
	int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

struct list_devs : list_parser<Device> {
	int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

class add_iface : public base_parser<const NetDevice&> {
	private:
		NetDevice* result;

	public:
		~add_iface();

		int valid(struct nl_msg* msg, const nl::attrs& attrs);

		const NetDevice& complete() { return *result; }
};

struct list_keys : list_parser<Key> {
	int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

struct list_seclevels : list_parser<Seclevel> {
	int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

}

#endif
