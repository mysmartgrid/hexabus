#include "nlparsers.hpp"

#include <string.h>

namespace parsers {

int list_phy::valid(struct nl_msg* msg, const nl::attrs& attrs)
{
	if (!attrs[IEEE802154_ATTR_PHY_NAME] ||
			!attrs[IEEE802154_ATTR_CHANNEL] ||
			!attrs[IEEE802154_ATTR_PAGE]) {
		std::cout << "abort" << std::endl;
		return NL_STOP;
	}

	std::string name = attrs.str(IEEE802154_ATTR_PHY_NAME);
	uint32_t channel = attrs.u8(IEEE802154_ATTR_CHANNEL);
	uint32_t page = attrs.u8(IEEE802154_ATTR_PAGE);
	std::vector<uint32_t> pages;

	if (attrs[IEEE802154_ATTR_CHANNEL_PAGE_LIST]) {
		int len = attrs.length(IEEE802154_ATTR_CHANNEL_PAGE_LIST);
		if (len % 4 != 0) {
			return NL_STOP;
		}

		const uint32_t *data = static_cast<const uint32_t*>(attrs.raw(IEEE802154_ATTR_CHANNEL_PAGE_LIST));

		for (int i = 0; i < len / 4; i++) {
			while (pages.size() <= data[i] >> 27)
				pages.push_back(0);
			pages[data[i] >> 27] = data[i] & 0x07FFFFFF;
		}
	}

	if (attrs[IEEE802154_ATTR_TXPOWER]) {
		int txpower = attrs.s8(IEEE802154_ATTR_TXPOWER);
		bool lbt = attrs.u8(IEEE802154_ATTR_LBT_ENABLED);
		uint8_t cca_mode = attrs.u8(IEEE802154_ATTR_CCA_MODE);
		int ed_level = attrs.s32(IEEE802154_ATTR_CCA_ED_LEVEL);

		list.push_back(Phy(name, page, channel, pages, txpower,
			cca_mode, ed_level, lbt));
	} else {
		list.push_back(Phy(name, page, channel, pages));
	}

	return NL_OK;
}



int add_iface::valid(struct nl_msg* msg, const nl::attrs& attrs)
{
	result = boost::make_optional(
		NetDevice(
			attrs.str(IEEE802154_ATTR_PHY_NAME),
			attrs.str(IEEE802154_ATTR_DEV_NAME)));

	return NL_OK;
}



int list_iface::valid(struct nl_msg* msg, const nl::attrs& attrs)
{
	NetDevice dev(
		attrs.str(IEEE802154_ATTR_PHY_NAME),
		attrs.str(IEEE802154_ATTR_DEV_NAME),
		attrs.u64(IEEE802154_ATTR_HW_ADDR),
		attrs.u16(IEEE802154_ATTR_SHORT_ADDR),
		attrs.u16(IEEE802154_ATTR_PAN_ID));

	list.push_back(dev);

	return NL_OK;
}



KeyLookupDescriptor parse_keydesc(const nl::attrs& attrs)
{
	uint8_t mode = attrs.u8(IEEE802154_ATTR_LLSEC_KEY_MODE);

	if (mode != 0) {
		uint8_t id = attrs.u8(IEEE802154_ATTR_LLSEC_KEY_ID);
		
		switch (mode) {
		case 1:
			return KeyLookupDescriptor(mode, id);

		case 2:
			return KeyLookupDescriptor(mode, id,
				attrs.u32(IEEE802154_ATTR_LLSEC_KEY_SOURCE_SHORT));

		case 3:
			return KeyLookupDescriptor(mode, id,
				attrs.u64(IEEE802154_ATTR_LLSEC_KEY_SOURCE_EXTENDED));
		}
	}

	return KeyLookupDescriptor(0);
}



int list_devs::valid(struct nl_msg* msg, const nl::attrs& attrs)
{
	std::string iface = attrs.str(IEEE802154_ATTR_DEV_NAME);

	if (this->iface.size() && iface != this->iface)
		return NL_OK;

	uint16_t pan_id = attrs.u16(IEEE802154_ATTR_PAN_ID);
	uint16_t short_addr = attrs.u16(IEEE802154_ATTR_SHORT_ADDR);
	uint64_t hwaddr = attrs.u64(IEEE802154_ATTR_HW_ADDR);
	uint32_t frame_ctr = attrs.u32(IEEE802154_ATTR_LLSEC_FRAME_COUNTER);
	KeyLookupDescriptor ldesc = parse_keydesc(attrs);

	list.push_back(Device(iface, pan_id, short_addr, hwaddr, frame_ctr, ldesc));

	return NL_OK;
}



int list_keys::valid(struct nl_msg* msg, const nl::attrs& attrs)
{
	std::string iface = attrs.str(IEEE802154_ATTR_DEV_NAME);

	if (this->iface.size() && iface != this->iface)
		return NL_OK;

	uint8_t mode = attrs.u8(IEEE802154_ATTR_LLSEC_KEY_MODE);
	uint8_t frames = attrs.u8(IEEE802154_ATTR_LLSEC_KEY_USAGE_FRAME_TYPES);
	uint8_t key[16];

	memcpy(key, attrs.raw(IEEE802154_ATTR_LLSEC_KEY_BYTES), 16);

	if (mode == 0) {
		list.push_back(Key::implicit(iface, frames, 0, key));
	} else {
		uint8_t id = attrs.u8(IEEE802154_ATTR_LLSEC_KEY_ID);

		if (mode == 1) {
			list.push_back(Key::indexed(iface, frames, 0, key, id));
		} else if (mode == 2) {
			list.push_back(Key::indexed(iface, frames, 0, key, id,
				attrs.u32(IEEE802154_ATTR_LLSEC_KEY_SOURCE_SHORT)));
		} else {
			list.push_back(Key::indexed(iface, frames, 0, key, id,
				attrs.u64(IEEE802154_ATTR_LLSEC_KEY_SOURCE_EXTENDED)));
		}
	}

	return NL_OK;
}



int list_seclevels::valid(struct nl_msg* msg, const nl::attrs& attrs)
{
	std::string iface = attrs.str(IEEE802154_ATTR_DEV_NAME);

	if (this->iface.size() && iface != this->iface)
		return NL_OK;

	list.push_back(Seclevel(
		iface,
		attrs.u8(IEEE802154_ATTR_LLSEC_FRAME_TYPE),
		attrs.u8(IEEE802154_ATTR_LLSEC_SECLEVELS)));

	return NL_OK;
}



int get_keydesc::valid(struct nl_msg* msg, const nl::attrs& attrs)
{
	result = parse_keydesc(attrs);

	return NL_OK;
}

}
