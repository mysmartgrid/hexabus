#include "binary-formatter.hpp"

#include <endian.h>
#include <string.h>

#include <stdexcept>

#include "exception.hpp"

void BinaryFormatter::get_raw(void* dest, std::size_t len)
{
	if (head + len > data.size())
		HXBNM_THROW(stream, "stream too short");

	memcpy(dest, &data[head], len);
	head += len;
}

void BinaryFormatter::put_raw(const void* src, std::size_t len)
{
	data.resize(data.size() + len);
	memcpy(&data[data.size() - len], src, len);
}

uint8_t BinaryFormatter::get_u8()
{
	uint8_t r;

	get_raw(&r, sizeof(r));
	return r;
}

uint16_t BinaryFormatter::get_u16()
{
	uint16_t r;

	get_raw(&r, sizeof(r));
	return be16toh(r);
}

uint32_t BinaryFormatter::get_u32()
{
	uint32_t r;

	get_raw(&r, sizeof(r));
	return be32toh(r);
}

uint64_t BinaryFormatter::get_u64()
{
	uint64_t r;

	get_raw(&r, sizeof(r));
	return be64toh(r);
}

void BinaryFormatter::put_u8(uint8_t u)
{
	put_raw(&u, sizeof(u));
}

void BinaryFormatter::put_u16(uint16_t u)
{
	u = htobe16(u);
	put_raw(&u, sizeof(u));
}

void BinaryFormatter::put_u32(uint32_t u)
{
	u = htobe32(u);
	put_raw(&u, sizeof(u));
}

void BinaryFormatter::put_u64(uint64_t u)
{
	u = htobe64(u);
	put_raw(&u, sizeof(u));
}
