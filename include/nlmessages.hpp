#ifndef NLMESSAGES_HPP_
#define NLMESSAGES_HPP_

#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <string>

#include "netlink.hpp"
#include "nl802154.h"

namespace msgs {

class msg802154 : public nl::msg {
	private:
		static int _family_id;

	public:
		static int family()
		{
			if (_family_id == 0)
				_family_id = nl::get_family(IEEE802154_NL_NAME);

			return _family_id;
		}

	public:
		msg802154(int flags, int cmd)
			: nl::msg(family(), 0, flags, cmd)
		{}
};

struct list_phy : msg802154 {
	list_phy(const std::string& iface = "")
		: msg802154(NLM_F_REQUEST | (iface.size() ? 0 : NLM_F_DUMP),
			IEEE802154_LIST_PHY)
	{
		if (iface.size())
			put(IEEE802154_ATTR_PHY_NAME, iface);
	}
};

struct add_iface : msg802154 {
	add_iface(const std::string& phy, const std::string& iface = "")
		: msg802154(NLM_F_REQUEST, IEEE802154_ADD_IFACE)
	{
		put(IEEE802154_ATTR_PHY_NAME, phy);
		if (iface.size())
			put(IEEE802154_ATTR_DEV_NAME, iface);

		put<uint8_t>(IEEE802154_ATTR_DEV_TYPE, IEEE802154_DEV_WPAN);
	}

	void hwaddr(uint64_t addr)
	{ put(IEEE802154_ATTR_HW_ADDR, addr); }
};

struct add_monitor : msg802154 {
	add_monitor(const std::string& phy, const std::string& iface = "")
		: msg802154(NLM_F_REQUEST, IEEE802154_ADD_IFACE)
	{
		put(IEEE802154_ATTR_PHY_NAME, phy);
		if (iface.size())
			put(IEEE802154_ATTR_DEV_NAME, iface);

		put<uint8_t>(IEEE802154_ATTR_DEV_TYPE, IEEE802154_DEV_MONITOR);
	}
};

struct list_iface : msg802154 {
	list_iface(const std::string& iface = "")
		: msg802154(NLM_F_REQUEST | (iface.size() ? 0 : NLM_F_DUMP),
				IEEE802154_LIST_IFACE)
	{
		if (iface.size())
			put(IEEE802154_ATTR_DEV_NAME, iface);
	}
};

struct del_iface : msg802154 {
	del_iface(const std::string& iface)
		: msg802154(NLM_F_REQUEST, IEEE802154_DEL_IFACE)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}
};

struct start : msg802154 {
	start(const std::string& dev)
		: msg802154(NLM_F_REQUEST, IEEE802154_START_REQ)
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

struct mac_setparams : msg802154 {
	mac_setparams(const std::string& dev)
		: msg802154(NLM_F_REQUEST, IEEE802154_SET_MACPARAMS)
	{
		put(IEEE802154_ATTR_DEV_NAME, dev);
	}

	void txpower(int32_t txp)
	{ put(IEEE802154_ATTR_TXPOWER, txp); }

	void lbt(bool enable)
	{ put<uint8_t>(IEEE802154_ATTR_LBT_ENABLED, enable); }

	void cca_mode(uint8_t mode)
	{ put(IEEE802154_ATTR_CCA_MODE, mode); }

	void ed_level(int32_t level)
	{ put(IEEE802154_ATTR_CCA_ED_LEVEL, level); }

	void frame_retries(int8_t retries)
	{ put(IEEE802154_ATTR_FRAME_RETRIES, retries); }
};

struct list_keys : msg802154 {
	list_keys()
		: msg802154(NLM_F_REQUEST | NLM_F_DUMP, IEEE802154_LLSEC_LIST_KEY)
	{}
};

struct modify_key : msg802154 {
	modify_key(int msg, const std::string iface)
		: msg802154(NLM_F_REQUEST, msg)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}

	void frames(uint8_t frames)
	{ put(IEEE802154_ATTR_LLSEC_KEY_USAGE_FRAME_TYPES, frames); }

	void id(uint8_t id)
	{ put(IEEE802154_ATTR_LLSEC_KEY_ID, id); }

	void mode(uint8_t mode)
	{ put(IEEE802154_ATTR_LLSEC_KEY_MODE, mode); }
};

struct add_key : modify_key {
	add_key(const std::string iface)
		: modify_key(IEEE802154_LLSEC_ADD_KEY, iface)
	{}

	void key(const void *key)
	{ put_raw(IEEE802154_ATTR_LLSEC_KEY_BYTES, key, 16); }
};

struct del_key : modify_key {
	del_key(const std::string iface)
		: modify_key(IEEE802154_LLSEC_DEL_KEY, iface)
	{}
};

struct list_devs : msg802154 {
	list_devs()
		: msg802154(NLM_F_REQUEST | NLM_F_DUMP, IEEE802154_LLSEC_LIST_DEV)
	{}
};

struct modify_dev : msg802154 {
	modify_dev(int msg, const std::string& iface)
		: msg802154(NLM_F_REQUEST, msg)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}

	void pan_id(uint16_t id)
	{ put(IEEE802154_ATTR_PAN_ID, id); }

	void short_addr(uint16_t addr)
	{ put(IEEE802154_ATTR_SHORT_ADDR, addr); }

	void hwaddr(uint64_t addr)
	{ put(IEEE802154_ATTR_HW_ADDR, addr); }

	void frame_ctr(uint32_t ctr)
	{ put(IEEE802154_ATTR_LLSEC_FRAME_COUNTER, ctr); }

	void sec_override(bool ovr)
	{ put(IEEE802154_ATTR_LLSEC_DEV_OVERRIDE, ovr); }

	void key_mode(uint8_t mode)
	{ put(IEEE802154_ATTR_LLSEC_DEV_KEY_MODE, mode); }
};

struct add_dev : modify_dev {
	add_dev(const std::string& iface)
		: modify_dev(IEEE802154_LLSEC_ADD_DEV, iface)
	{}
};

struct del_dev : modify_dev {
	del_dev(const std::string& iface)
		: modify_dev(IEEE802154_LLSEC_DEL_DEV, iface)
	{}
};

struct list_devkeys : msg802154 {
	list_devkeys()
		: msg802154(NLM_F_REQUEST | NLM_F_DUMP, IEEE802154_LLSEC_LIST_DEVKEY)
	{}
};

struct modify_devkey : msg802154 {
	modify_devkey(int msg, const std::string& iface)
		: msg802154(NLM_F_REQUEST, msg)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}

	void device(uint64_t addr)
	{ put(IEEE802154_ATTR_HW_ADDR, addr); }

	void frame_counter(uint32_t ctr)
	{ put(IEEE802154_ATTR_LLSEC_FRAME_COUNTER, ctr); }

	void key_id(uint8_t id)
	{ put(IEEE802154_ATTR_LLSEC_KEY_ID, id); }

	void key_mode(uint8_t mode)
	{ put(IEEE802154_ATTR_LLSEC_KEY_MODE, mode); }
};

struct add_devkey : modify_devkey {
	add_devkey(const std::string& iface)
		: modify_devkey(IEEE802154_LLSEC_ADD_DEVKEY, iface)
	{}
};

struct del_devkey : modify_devkey {
	del_devkey(const std::string& iface)
		: modify_devkey(IEEE802154_LLSEC_DEL_DEVKEY, iface)
	{}
};

struct list_seclevels : msg802154 {
	list_seclevels()
		: msg802154(NLM_F_REQUEST | NLM_F_DUMP, IEEE802154_LLSEC_LIST_SECLEVEL)
	{}
};

struct modify_seclevel : msg802154 {
	modify_seclevel(int msg, const std::string& iface)
		: msg802154(NLM_F_REQUEST, msg)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}

	void frame(uint8_t id)
	{ put(IEEE802154_ATTR_LLSEC_FRAME_TYPE, id); }

	void levels(uint8_t levels)
	{ put(IEEE802154_ATTR_LLSEC_SECLEVELS, levels); }

	void dev_override(bool ovr)
	{ put(IEEE802154_ATTR_LLSEC_DEV_OVERRIDE, ovr); }
};

struct add_seclevel : modify_seclevel {
	add_seclevel(const std::string& iface)
		: modify_seclevel(IEEE802154_LLSEC_ADD_SECLEVEL, iface)
	{}
};

struct del_seclevel : modify_seclevel {
	del_seclevel(const std::string& iface)
		: modify_seclevel(IEEE802154_LLSEC_DEL_SECLEVEL, iface)
	{}
};

struct llsec_setparams : msg802154 {
	llsec_setparams(const std::string& iface)
		: msg802154(NLM_F_REQUEST, IEEE802154_LLSEC_SETPARAMS)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}

	void enabled(bool on)
	{ put(IEEE802154_ATTR_LLSEC_ENABLED, on); }

	void out_level(uint8_t level)
	{ put(IEEE802154_ATTR_LLSEC_SECLEVEL, level); }

	void key_id(uint8_t id)
	{ put(IEEE802154_ATTR_LLSEC_KEY_ID, id); }

	void key_mode(uint8_t mode)
	{ put(IEEE802154_ATTR_LLSEC_KEY_MODE, mode); }

	void key_source_short(uint32_t id)
	{ put(IEEE802154_ATTR_LLSEC_KEY_SOURCE_SHORT, id); }

	void key_source_hw(uint64_t hw)
	{ put(IEEE802154_ATTR_LLSEC_KEY_SOURCE_EXTENDED, hw); }

	void frame_counter(uint32_t fc)
	{ put(IEEE802154_ATTR_LLSEC_FRAME_COUNTER, fc); }
};

struct llsec_getparams : msg802154 {
	llsec_getparams(const std::string& iface)
		: msg802154(NLM_F_REQUEST, IEEE802154_LLSEC_GETPARAMS)
	{
		put(IEEE802154_ATTR_DEV_NAME, iface);
	}
};

}

#endif
