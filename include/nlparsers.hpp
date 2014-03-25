#ifndef NLPARSERS_HPP_
#define NLPARSERS_HPP_

#include <string>
#include <boost/optional.hpp>

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

class add_iface : public base_parser<const NetDevice&> {
	private:
		boost::optional<NetDevice> result;

	public:
		int valid(struct nl_msg* msg, const nl::attrs& attrs);

		const NetDevice& complete() { return *result; }
};

struct list_iface : public list_parser<NetDevice> {
	int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

class list_devs : public list_parser<Device> {
	private:
		std::string iface;

	public:
		list_devs(const std::string& iface = "")
			: iface(iface)
		{}

		int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

class list_keys : public list_parser<Key> {
	private:
		std::string iface;

	public:
		list_keys(const std::string& iface = "")
			: iface(iface)
		{}

		int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

class list_seclevels : public list_parser<Seclevel> {
	private:
		std::string iface;

	public:
		list_seclevels(const std::string& iface = "")
			: iface(iface)
		{}

		int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

class get_keydesc : public base_parser<const KeyLookupDescriptor&> {
	private:
		boost::optional<KeyLookupDescriptor> result;

	public:
		int valid(struct nl_msg* msg, const nl::attrs& attrs);

		const KeyLookupDescriptor& complete() { return *result; }
};

}

#endif
