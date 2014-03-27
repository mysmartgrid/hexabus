#include "controller.hpp"

#include <algorithm>
#include <stdexcept>

#include <boost/foreach.hpp>

#include "nlparsers.hpp"
#include "nlmessages.hpp"

Controller::Controller()
	: socket(msgs::msg802154::family())
{
}

std::vector<Phy> Controller::list_phys()
{
	msgs::list_phy cmd;
	parsers::list_phy p;

	return send(cmd, p);
}

std::vector<NetDevice> Controller::list_netdevs(const std::string& iface)
{
	msgs::list_iface cmd(iface);
	parsers::list_iface p;

	return send(cmd, p);
}

Controller::dev_list_t Controller::list_devices()
{
	msgs::list_devs cmd;
	parsers::list_devs p;

	return send(cmd, p);
}

std::vector<Device> Controller::list_devices(const std::string& iface)
{
	msgs::list_devs cmd;
	parsers::list_devs p(iface);

	dev_list_t devs = send(cmd, p);

	std::vector<Device> result;
	result.reserve(devs.size());
	BOOST_FOREACH(const dev_list_t::value_type& p, devs) {
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

void Controller::setup_phy(const std::string& phy)
{
	msgs::phy_setparams cmd(phy);

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
	akey.key(key.key());
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

	send(msg);
}

void Controller::add_device(const std::string& iface, const Device& dev)
{
	msgs::add_dev cmd(iface);

	cmd.pan_id(dev.pan_id());
	cmd.short_addr(dev.short_addr());
	cmd.hwaddr(dev.hwaddr());
	cmd.frame_ctr(dev.frame_ctr());

	send(cmd);
}
