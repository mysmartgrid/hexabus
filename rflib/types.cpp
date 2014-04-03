#include "types.hpp"

Key Key::implicit(uint8_t frames, uint8_t cmd_frames, const uint8_t* key)
{
	return Key(KeyLookupDescriptor(0), frames, cmd_frames, key);
}

Key Key::indexed(uint8_t frames, uint8_t cmd_frames, const uint8_t* key, uint8_t id)
{
	return Key(KeyLookupDescriptor(1, id), frames, cmd_frames, key);
}

Key Key::indexed(uint8_t frames, uint8_t cmd_frames, const uint8_t* key, uint8_t id,
		uint32_t source)
{
	return Key(KeyLookupDescriptor(2, id, source), frames, cmd_frames, key);
}

Key Key::indexed(uint8_t frames, uint8_t cmd_frames, const uint8_t* key, uint8_t id,
		uint64_t source)
{
	return Key(KeyLookupDescriptor(3, id, source), frames, cmd_frames, key);
}



ieee802154_addr NetDevice::addr_raw() const
{
	ieee802154_addr result;
	uint64_t hwaddr = addr();

	result.addr_type = IEEE802154_ADDR_LONG;
	result.pan_id = pan_id();
	memcpy(result.hwaddr, &hwaddr, sizeof(hwaddr));

	return result;
}
