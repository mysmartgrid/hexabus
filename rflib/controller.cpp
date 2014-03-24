#include "controller.hpp"

#include <algorithm>
#include <stdexcept>

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

std::vector<Device> Controller::list_devices(const std::string& iface)
{
	msgs::list_devs cmd;
	parsers::list_devs p(iface);

	std::vector<Device> devs = send(cmd, p);

	return devs;
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

std::vector<Key> Controller::list_keys(const std::string& iface)
{
	msgs::list_keys cmd;
	parsers::list_keys p(iface);

	return send(cmd, p);
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

void Controller::add_key(const Key& key)
{
	msgs::add_key akey(key.iface());

	akey.frames(key.frame_types());
	akey.key(key.key());
	akey.mode(key.mode());

	if (key.mode() != 0) {
		akey.id(key.id());
		if (key.mode() != 1) {
			// TODO: implement
			throw std::runtime_error("not implemented");
		}
	}

	send(akey);
}

void Controller::enable_security(const std::string& dev)
{
	{
		msgs::llsec_setparams msg(dev);

		msg.enabled(true);
		msg.out_level(5);
		msg.key_id(0);
		msg.key_mode(1);
		msg.key_source_hw(std::numeric_limits<uint64_t>::max());

		send(msg);
	} {
		msgs::add_seclevel msg(dev);

		msg.frame(1);
		msg.levels(0xff);

		send(msg);
	}
}

void Controller::add_device(const Device& dev)
{
	msgs::add_dev cmd(dev.iface());

	cmd.pan_id(dev.pan_id());
	cmd.short_addr(dev.short_addr());
	cmd.hwaddr(dev.hwaddr());
	cmd.frame_ctr(dev.frame_ctr());

	send(cmd);
}
