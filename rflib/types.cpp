#include "types.hpp"

Key Key::implicit(const std::string& iface, uint8_t frames, uint8_t cmd_frames,
	const uint8_t* key)
{
	Key result;

	result._iface = iface;
	result._frame_types = frames;
	result._cmd_frame_ids = cmd_frames;
	result._mode = 0;
	memcpy(result._key, key, 16);
	return result;
}

Key Key::indexed(const std::string& iface, uint8_t frames, uint8_t cmd_frames,
	const uint8_t* key, uint8_t id)
{
	Key result = implicit(iface, frames, cmd_frames, key);

	result._id = id;
	result._mode = 1;
	return result;
}

Key Key::indexed(const std::string& iface, uint8_t frames, uint8_t cmd_frames,
	const uint8_t* key, uint8_t id, uint32_t source)
{
	Key result = indexed(iface, frames, cmd_frames, key, id);

	result._source = boost::variant<uint32_t, uint64_t>(source);
	result._mode = 2;
	return result;
}

Key Key::indexed(const std::string& iface, uint8_t frames, uint8_t cmd_frames,
	const uint8_t* key, uint8_t id, uint64_t source)
{
	Key result = indexed(iface, frames, cmd_frames, key, id);

	result._source = boost::variant<uint32_t, uint64_t>(source);
	result._mode = 3;
	return result;
}
