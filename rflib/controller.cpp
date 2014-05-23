#include "controller.hpp"

#include <algorithm>
#include <stdexcept>

#include <netlink/route/rtnl.h>
#include <netlink/route/link.h>

#include <boost/foreach.hpp>

#include "nlparsers.hpp"
#include "nlmessages.hpp"

Controller::Controller()
	: socket(msgs::msg802154::family())
{
}

std::vector<Phy> Controller::list_phys(const std::string& name)
{
	msgs::list_phy cmd(name);
	parsers::list_phy p;

	return send(cmd, p);
}

std::vector<NetDevice> Controller::list_netdevs(const std::string& iface)
{
	msgs::list_iface cmd(iface);
	parsers::list_iface p;

	std::vector<NetDevice> result = send(cmd, p);
	if (iface.size())
		recv();

	return result;
}

Controller::dev_list_t Controller::list_devices()
{
	msgs::list_devs cmd;
	parsers::list_devs p;

	dev_list_t devs(send(cmd, p));

	std::map<std::pair<uint64_t, std::string>, Device*> devs_by_addr;
	BOOST_FOREACH(dev_list_t::value_type& dev, devs) {
		devs_by_addr[std::make_pair(dev.first.hwaddr(), dev.second)] = &dev.first;
	}

	msgs::list_devkeys cmd2;
	parsers::list_devkeys p2;

	std::vector<parsers::devkey> keys(send(cmd2, p2));

	BOOST_FOREACH(const parsers::devkey& key, keys) {
		std::pair<uint64_t, std::string> idx(key.dev_addr, key.iface);

		if (!devs_by_addr.count(idx))
			continue;

		devs_by_addr[idx]->keys().insert(std::make_pair(key.key, key.frame_ctr));
	}

	return devs;
}

std::vector<Device> Controller::list_devices(const std::string& iface)
{
	dev_list_t devs = list_devices();

	std::vector<Device> result;
	BOOST_FOREACH(const dev_list_t::value_type& p, devs) {
		if (p.second == iface)
			result.push_back(p.first);
	}

	return result;
}

struct dev_addr_filter {
	uint64_t addr;
	bool operator()(const Device& dev) const
	{ return dev.hwaddr() != addr; }
};

std::vector<Device> Controller::list_devices(const std::string& iface, uint64_t addr)
{
	std::vector<Device> devs = list_devices(iface);
	dev_addr_filter filter = { addr };

	devs.erase(std::remove_if(devs.begin(), devs.end(), filter), devs.end());

	return devs;
}

Controller::sl_list_t Controller::list_seclevels()
{
	msgs::list_seclevels cmd;
	parsers::list_seclevels p;

	return send(cmd, p);
}

std::vector<Seclevel> Controller::list_seclevels(const std::string& iface)
{
	msgs::list_seclevels cmd;
	parsers::list_seclevels p(iface);

	sl_list_t levels = send(cmd, p);

	std::vector<Seclevel> result;
	result.reserve(levels.size());
	BOOST_FOREACH(const sl_list_t::value_type& p, levels) {
		result.push_back(p.first);
	}

	return result;
}

Controller::key_list_t Controller::list_keys()
{
	msgs::list_keys cmd;
	parsers::list_keys p;

	return send(cmd, p);
}

std::vector<Key> Controller::list_keys(const std::string& iface)
{
	msgs::list_keys cmd;
	parsers::list_keys p(iface);

	key_list_t keys = send(cmd, p);

	std::vector<Key> result;
	result.reserve(keys.size());
	BOOST_FOREACH(const key_list_t::value_type& p, keys) {
		result.push_back(p.first);
	}

	return result;
}

void Controller::setup_dev(const std::string& dev)
{
	msgs::mac_setparams cmd(dev);

	cmd.lbt(true);
	cmd.cca_mode(0);
	cmd.ed_level(-81);
	cmd.txpower(3);
	cmd.frame_retries(3);

	send(cmd);
}

void Controller::start(const std::string& dev, const PAN& pan, uint16_t short_addr)
{
	msgs::start start(dev);

	start.short_addr(short_addr);
	start.pan_id(pan.pan_id());
	start.channel(pan.channel());
	start.page(pan.page());

	send(start);
}

NetDevice Controller::add_netdev(const Phy& phy, uint64_t addr, const std::string& name)
{
	std::string pattern = name.size() ? name : "wpan%d";

	msgs::add_iface cmd(phy.name(), pattern);
	parsers::add_iface pi;

	cmd.hwaddr(addr);
	NetDevice dev = send(cmd, pi);
	recv();

	return dev;
}

NetDevice Controller::add_monitor(const std::string& phy, const std::string& name)
{
	std::string pattern = name.size() ? name : "wpanmon%d";

	msgs::add_monitor cmd(phy, pattern);
	parsers::add_iface pi;

	NetDevice dev = send(cmd, pi);
	recv();

	return dev;
}

void Controller::remove_netdev(const std::string& name)
{
	msgs::del_iface cmd(name);

	send(cmd);
	recv();
}

void Controller::add_key(const std::string& iface, const Key& key)
{
	msgs::add_key akey(iface);

	akey.frames(key.frame_types());
	akey.key(key.key().c_array());
	akey.mode(key.lookup_desc().mode());

	if (key.lookup_desc().mode() != 0) {
		akey.id(key.lookup_desc().id());
		if (key.lookup_desc().mode() != 1) {
			// TODO: implement
			throw std::runtime_error("not implemented");
		}
	}

	send(akey);
}

void Controller::setparams(const std::string& dev, const SecurityParameters& params)
{
	msgs::llsec_setparams msg(dev);

	if (params.out_key().mode() != 1)
		throw std::runtime_error("invalid key");

	msg.enabled(params.enabled());
	msg.out_level(params.out_level());
	msg.key_id(params.out_key().id());
	msg.key_mode(params.out_key().mode());
	switch (params.out_key().mode()) {
	case 2:
		msg.key_source_short(boost::get<uint32_t>(params.out_key().source()));
		break;

	case 3:
		msg.key_source_hw(boost::get<uint64_t>(params.out_key().source()));
		break;
	}
	msg.frame_counter(params.frame_counter());

	send(msg);
}

SecurityParameters Controller::getparams(const std::string& dev)
{
	msgs::llsec_getparams msg(dev);
	parsers::get_secparams p;

	SecurityParameters params = send(msg, p);
	recv();

	return params;
}

Key Controller::get_out_key(const std::string& iface)
{
	KeyLookupDescriptor desc = getparams(iface).out_key();

	std::vector<Key> keys = list_keys(iface);
	for (std::vector<Key>::const_iterator it = keys.begin(), end = keys.end();
		it != end; it++) {
		if (it->lookup_desc() == desc)
			return *it;
	}

	throw std::runtime_error("key not found");
}

void Controller::add_seclevel(const std::string& dev, const Seclevel& sl)
{
	msgs::add_seclevel msg(dev);

	msg.frame(sl.frame_type());
	msg.levels(sl.levels());
	msg.dev_override(false);

	send(msg);
}

void Controller::add_device(const std::string& iface, const Device& dev)
{
	msgs::add_dev cmd(iface);

	cmd.pan_id(dev.pan_id());
	cmd.short_addr(dev.short_addr());
	cmd.hwaddr(dev.hwaddr());
	cmd.frame_ctr(dev.frame_ctr());
	cmd.sec_override(dev.sec_override());
	cmd.key_mode(dev.key_mode());

	send(cmd);

	typedef std::map<KeyLookupDescriptor, uint32_t>::const_iterator cit;
	for (cit it = dev.keys().begin(), end = dev.keys().end(); it != end; ++it) {
		msgs::add_devkey cmd(iface);

		cmd.device(dev.hwaddr());
		cmd.frame_counter(it->second);
		cmd.key_mode(it->first.mode());
		cmd.key_id(it->first.id());

		send(cmd);
	}
}

void Controller::remove_device(const std::string& iface, uint64_t hwaddr)
{
	msgs::del_dev cmd(iface);

	cmd.hwaddr(hwaddr);

	send(cmd);
}



struct rtnl_sock {
	nl_sock* sock;

	rtnl_sock()
		: sock(nl_socket_alloc())
	{
		if (!sock)
			throw std::runtime_error("can't alloce rtnl socket");
		nl_connect(sock, NETLINK_ROUTE);
	}

	~rtnl_sock()
	{
		nl_socket_free(sock);
	}
};

struct rtnl_link {
	rtnl_link* link;

	rtnl_link()
		: link(rtnl_link_alloc())
	{
		if (!link)
			throw std::runtime_error("can't alloc rtnl link");
	}

	~rtnl_link()
	{
		rtnl_link_put(link);
	}
};

// this is easier than making sense of the include error mess
extern "C" unsigned int if_nametoindex(const char*);

void create_lowpan_device(const std::string& master, const std::string& lowpan)
{
	rtnl_sock sock;
	rtnl_link link;
	int rc;
	int master_index;

	master_index = if_nametoindex(master.c_str());
	if (!master_index)
		throw std::runtime_error(master + " not found");

	rtnl_link_set_name(link.link, lowpan.c_str());
	rtnl_link_set_type(link.link, "lowpan");
	rtnl_link_set_link(link.link, master_index);

	rc = rtnl_link_add(sock.sock, link.link, NLM_F_EXCL | NLM_F_CREATE);
	if (rc < 0)
		throw std::runtime_error(nl_geterror(rc));
}
