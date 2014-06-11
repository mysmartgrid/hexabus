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

class list_devs : public list_parser<std::pair<Device, std::string> > {
	private:
		std::string iface;

	public:
		list_devs(const std::string& iface = "")
			: iface(iface)
		{}

		int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

struct devkey {
	std::string iface;
	uint64_t dev_addr;
	uint32_t frame_ctr;
	KeyLookupDescriptor key;
};

class list_devkeys : public list_parser<devkey> {
	private:
		std::string iface;

	public:
		list_devkeys(const std::string& iface = "")
			: iface(iface)
		{}

		int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

class list_keys : public list_parser<std::pair<Key, std::string> > {
	private:
		std::string iface;

	public:
		list_keys(const std::string& iface = "")
			: iface(iface)
		{}

		int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

class list_seclevels : public list_parser<std::pair<Seclevel, std::string> > {
	private:
		std::string iface;

	public:
		list_seclevels(const std::string& iface = "")
			: iface(iface)
		{}

		int valid(struct nl_msg* msg, const nl::attrs& attrs);
};

class get_secparams : public base_parser<const SecurityParameters&> {
	private:
		boost::optional<SecurityParameters> result;

	public:
		int valid(struct nl_msg* msg, const nl::attrs& attrs);

		const SecurityParameters& complete() { return *result; }
};

}

#endif
