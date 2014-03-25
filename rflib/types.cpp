#include "types.hpp"

Key Key::implicit(const std::string& iface, uint8_t frames, uint8_t cmd_frames,
	const uint8_t* key)
{
	return Key(iface, KeyLookupDescriptor(0), frames, cmd_frames, key);
}

Key Key::indexed(const std::string& iface, uint8_t frames, uint8_t cmd_frames,
	const uint8_t* key, uint8_t id)
{
	return Key(iface, KeyLookupDescriptor(1, id), frames, cmd_frames, key);
}

Key Key::indexed(const std::string& iface, uint8_t frames, uint8_t cmd_frames,
	const uint8_t* key, uint8_t id, uint32_t source)
{
	return Key(iface, KeyLookupDescriptor(2, id, source), frames, cmd_frames, key);
}

Key Key::indexed(const std::string& iface, uint8_t frames, uint8_t cmd_frames,
	const uint8_t* key, uint8_t id, uint64_t source)
{
	return Key(iface, KeyLookupDescriptor(3, id, source), frames, cmd_frames, key);
}
