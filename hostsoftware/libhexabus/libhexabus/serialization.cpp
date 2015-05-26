#include <libhexabus/private/serialization.hpp>

#include <algorithm>
#include <cstring>
#include <endian.h>
#include <stdexcept>

#include "crc.hpp"
#include "error.hpp"

#include "../../../shared/hexabus_definitions.h"

using namespace hexabus;

static_assert(std::numeric_limits<float>::is_iec559, "invalid float");
static_assert(sizeof(char) == sizeof(uint8_t), "invalid char");

// {{{ Binary serialization visitor

class BinarySerializer : public PacketVisitor {
	private:
		std::vector<char>& _target;
		size_t _headerStart;

		template<typename B>
		void appendBlock(B block)
		{
			size_t size = _target.size();
			_target.insert(_target.end(), sizeof(B), 0);
			memcpy(&_target[size], &block, sizeof(B));
		}

		static uint64_t signedToBits(int64_t val)
		{
			uint64_t result;
			memcpy(&result, &val, sizeof(val));
			return result;
		}

		static uint32_t floatToBits(float val)
		{
			uint32_t result;
			memcpy(&result, &val, sizeof(val));
			return result;
		}

		void append_u8(uint8_t value) { appendBlock<uint8_t>(value); }
		void append_u16(uint16_t value) { appendBlock<uint16_t>(htobe16(value)); }
		void append_u32(uint32_t value) { appendBlock<uint32_t>(htobe32(value)); }
		void append_u64(uint64_t value) { appendBlock<uint64_t>(htobe64(value)); }
		void append_s8(int8_t value) { append_u8(signedToBits(value)); }
		void append_s16(int16_t value) { append_u16(signedToBits(value)); }
		void append_s32(int32_t value) { append_u32(signedToBits(value)); }
		void append_s64(int64_t value) { append_u64(signedToBits(value)); }
		void append_float(float value) { append_u32(floatToBits(value)); }

		void appendHeader(const Packet& packet);
		void appendEIDHeader(const EIDPacket& packet);
		void appendValueHeader(const TypedPacket& packet);

		void appendValue(const ValuePacket<bool>& value);
		void appendValue(const ValuePacket<uint8_t>& value);
		void appendValue(const ValuePacket<uint16_t>& value);
		void appendValue(const ValuePacket<uint32_t>& value);
		void appendValue(const ValuePacket<uint64_t>& value);
		void appendValue(const ValuePacket<int8_t>& value);
		void appendValue(const ValuePacket<int16_t>& value);
		void appendValue(const ValuePacket<int32_t>& value);
		void appendValue(const ValuePacket<int64_t>& value);
		void appendValue(const ValuePacket<float>& value);
		void appendValue(const ValuePacket<std::string>& value);
		void appendValue(const ValuePacket<std::array<uint8_t, 65> >& value);
		void appendValue(const ValuePacket<std::array<uint8_t, 16> >& value);

		void appendCRC();

	public:
		BinarySerializer(std::vector<char>& target);

		virtual void visit(const ErrorPacket& error);
		virtual void visit(const QueryPacket& query);
		virtual void visit(const EndpointQueryPacket& endpointQuery);
		virtual void visit(const EndpointInfoPacket& endpointInfo) { appendValue(endpointInfo); }

		virtual void visit(const InfoPacket<bool>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<uint8_t>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<uint16_t>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<uint32_t>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<uint64_t>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<int8_t>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<int16_t>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<int32_t>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<int64_t>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<float>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<std::string>& info) { appendValue(info); }
		virtual void visit(const InfoPacket<std::array<uint8_t, 16> >& info) { appendValue(info); }
		virtual void visit(const InfoPacket<std::array<uint8_t, 65> >& info) { appendValue(info); }

		virtual void visit(const WritePacket<bool>& write) { appendValue(write); }
		virtual void visit(const WritePacket<uint8_t>& write) { appendValue(write); }
		virtual void visit(const WritePacket<uint16_t>& write) { appendValue(write); }
		virtual void visit(const WritePacket<uint32_t>& write) { appendValue(write); }
		virtual void visit(const WritePacket<uint64_t>& write) { appendValue(write); }
		virtual void visit(const WritePacket<int8_t>& write) { appendValue(write); }
		virtual void visit(const WritePacket<int16_t>& write) { appendValue(write); }
		virtual void visit(const WritePacket<int32_t>& write) { appendValue(write); }
		virtual void visit(const WritePacket<int64_t>& write) { appendValue(write); }
		virtual void visit(const WritePacket<float>& write) { appendValue(write); }
		virtual void visit(const WritePacket<std::string>& write) { appendValue(write); }
		virtual void visit(const WritePacket<std::array<uint8_t, 65> >& write) { appendValue(write); }
		virtual void visit(const WritePacket<std::array<uint8_t, 16> >& write) { appendValue(write); }
};

BinarySerializer::BinarySerializer(std::vector<char>& target)
	: _target(target)
{
}

void BinarySerializer::appendHeader(const Packet& packet)
{
	_headerStart = _target.size();

	_target.insert(_target.end(), HXB_HEADER, HXB_HEADER + strlen(HXB_HEADER));
	_target.push_back(packet.type());
	_target.push_back(packet.flags());
}

void BinarySerializer::appendEIDHeader(const EIDPacket& packet)
{
	appendHeader(packet);
	append_u32(packet.eid());
}

void BinarySerializer::appendValueHeader(const TypedPacket& packet)
{
	appendEIDHeader(packet);
	append_u8(packet.datatype());
}

void BinarySerializer::appendCRC()
{
	uint16_t crc = hexabus::crc(&_target[_headerStart], _target.size() - _headerStart);

	append_u16(crc);
}

void BinarySerializer::visit(const ErrorPacket& error)
{
	appendHeader(error);
	append_u8(error.code());
	appendCRC();
}

void BinarySerializer::visit(const QueryPacket& query)
{
	appendEIDHeader(query);
	appendCRC();
}

void BinarySerializer::visit(const EndpointQueryPacket& endpointQuery)
{
	appendEIDHeader(endpointQuery);
	appendCRC();
}

#define BS_APPEND_VALUE(TYPE, APPEND) \
	void BinarySerializer::appendValue(const ValuePacket<TYPE>& value) \
	{ \
		appendValueHeader(value); \
		APPEND(value.value()); \
		appendCRC(); \
	}

BS_APPEND_VALUE(bool, append_u8)
BS_APPEND_VALUE(uint8_t, append_u8)
BS_APPEND_VALUE(uint16_t, append_u16)
BS_APPEND_VALUE(uint32_t, append_u32)
BS_APPEND_VALUE(uint64_t, append_u64)
BS_APPEND_VALUE(int8_t, append_s8)
BS_APPEND_VALUE(int16_t, append_s16)
BS_APPEND_VALUE(int32_t, append_s32)
BS_APPEND_VALUE(int64_t, append_s64)
BS_APPEND_VALUE(float, append_float)

#undef BS_APPEND_VALUE

void BinarySerializer::appendValue(const ValuePacket<std::string>& value)
{
	appendValueHeader(value);

	_target.insert(_target.end(), value.value().begin(), value.value().end());
	_target.insert(_target.end(), ValuePacket<std::string>::max_length + 1 - value.value().size(), '\0');

	appendCRC();
}

void BinarySerializer::appendValue(const ValuePacket<std::array<uint8_t, 16> >& value)
{
	appendValueHeader(value);

	_target.resize(_target.size() + value.value().size());
	std::memcpy(&_target[_target.size() - value.value().size()], &value.value()[0], value.value().size());

	appendCRC();
}

void BinarySerializer::appendValue(const ValuePacket<std::array<uint8_t, 65> >& value)
{
	appendValueHeader(value);

	_target.resize(_target.size() + value.value().size());
	std::memcpy(&_target[_target.size() - value.value().size()], &value.value()[0], value.value().size());

	appendCRC();
}

// }}}

std::vector<char> hexabus::serialize(const Packet& packet)
{
	std::vector<char> result;
	BinarySerializer serializer(result);

	serializer.visitPacket(packet);

	return result;
}

// {{{ Binary deserialization

class BinaryDeserializer {
	private:
		const char* _packet;
		size_t _offset;
		size_t _size;

		void checkLength(size_t min);

		void readHeader();

		template<typename B>
		B readBlock()
		{
			checkLength(sizeof(B));

			B data;
			memcpy(&data, _packet + _offset, sizeof(B));
			_offset += sizeof(B);
			return data;
		}

		int64_t signedFromBits(uint64_t val)
		{
			int64_t result;
			memcpy(&result, &val, sizeof(val));
			return result;
		}

		static float floatFromBits(uint32_t val)
		{
			float result;
			memcpy(&result, &val, sizeof(val));
			return result;
		}

		uint8_t read_u8() { return readBlock<uint8_t>(); }
		uint16_t read_u16() { return be16toh(readBlock<uint16_t>()); }
		uint32_t read_u32() { return be32toh(readBlock<uint32_t>()); }
		uint64_t read_u64() { return be64toh(readBlock<uint64_t>()); }
		int8_t read_s8() { return signedFromBits(read_u8()); }
		int16_t read_s16() { return signedFromBits(read_u16()); }
		int32_t read_s32() { return signedFromBits(read_u32()); }
		int64_t read_s64() { return signedFromBits(read_u64()); }
		float read_float() { return floatFromBits(read_u32()); }

		template<size_t L>
		std::array<uint8_t, L> read_bytes();
		std::string read_string();

		template<typename T>
		Packet::Ptr checkInfo(bool info, uint32_t eid, const T& value, uint8_t flags);

		template<typename T>
		Packet::Ptr check(const T& packet);

	public:
		BinaryDeserializer(const char* packet, size_t size)
			: _packet(packet), _offset(0), _size(size)
		{
			if (!packet)
				throw std::invalid_argument("packet");
		}

		Packet::Ptr deserialize();
};

void BinaryDeserializer::checkLength(size_t min)
{
	if (_size - _offset < min)
		throw BadPacketException("Packet too short");
}

void BinaryDeserializer::readHeader()
{
	checkLength(strlen(HXB_HEADER));

	if (memcmp(HXB_HEADER, _packet + _offset, strlen(HXB_HEADER)))
		throw BadPacketException("Invalid header");

	_offset += strlen(HXB_HEADER);
}

template<size_t L>
std::array<uint8_t, L> BinaryDeserializer::read_bytes()
{
	checkLength(L);

	std::array<uint8_t, L> result;
	std::memcpy(&result[0], _packet + _offset, L);
	_offset += L;

	return result;
}

std::string BinaryDeserializer::read_string()
{
	checkLength(ValuePacket<std::string>::max_length);

	if (!std::find(_packet + _offset, _packet + _offset + ValuePacket<std::string>::max_length, '\0'))
		throw BadPacketException("Unterminated string");

	std::string result(_packet + _offset);
	_offset += ValuePacket<std::string>::max_length + 1;

	return result;
}

template<typename T>
Packet::Ptr BinaryDeserializer::checkInfo(bool info, uint32_t eid, const T& value, uint8_t flags)
{
	if (info) {
		return check(InfoPacket<T>(eid, value, flags));
	} else {
		return check(WritePacket<T>(eid, value, flags));
	}
}

template<typename T>
Packet::Ptr BinaryDeserializer::check(const T& packet)
{
	uint16_t crc = hexabus::crc(_packet, _offset);
	if (crc != read_u16()) {
		throw BadPacketException("Bad checksum");
	}

	return Packet::Ptr(new T(packet));
}

Packet::Ptr BinaryDeserializer::deserialize()
{
	readHeader();

	uint8_t type = read_u8();
	uint8_t flags = read_u8();

	switch (type) {
		case HXB_PTYPE_ERROR:
			{
				uint8_t code = read_u8();
				return check(ErrorPacket(code, flags));
			}

		case HXB_PTYPE_INFO:
		case HXB_PTYPE_WRITE:
			{
				bool info = type == HXB_PTYPE_INFO;
				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();

				switch (datatype) {
					case HXB_DTYPE_BOOL: return checkInfo<bool>(info, eid, read_u8(), flags);
					case HXB_DTYPE_UINT8: return checkInfo<uint8_t>(info, eid, read_u8(), flags);
					case HXB_DTYPE_UINT16: return checkInfo<uint16_t>(info, eid, read_u16(), flags);
					case HXB_DTYPE_UINT32: return checkInfo<uint32_t>(info, eid, read_u32(), flags);
					case HXB_DTYPE_UINT64: return checkInfo<uint64_t>(info, eid, read_u64(), flags);
					case HXB_DTYPE_SINT8: return checkInfo<int8_t>(info, eid, read_s8(), flags);
					case HXB_DTYPE_SINT16: return checkInfo<int16_t>(info, eid, read_s16(), flags);
					case HXB_DTYPE_SINT32: return checkInfo<int32_t>(info, eid, read_s32(), flags);
					case HXB_DTYPE_SINT64: return checkInfo<int64_t>(info, eid, read_s64(), flags);
					case HXB_DTYPE_FLOAT: return checkInfo<float>(info, eid, read_float(), flags);
					case HXB_DTYPE_128STRING: return checkInfo<std::string>(info, eid, read_string(), flags);
					case HXB_DTYPE_16BYTES: return checkInfo<std::array<uint8_t, 16> >(info, eid, read_bytes<16>(), flags);
					case HXB_DTYPE_65BYTES: return checkInfo<std::array<uint8_t, 65> >(info, eid, read_bytes<65>(), flags);

					default:
						throw BadPacketException("Invalid datatype");
				}
			}

		case HXB_PTYPE_QUERY:
			{
				uint32_t eid = read_u32();
				return check(QueryPacket(eid, flags));
			}

		case HXB_PTYPE_EPQUERY:
			{
				uint32_t eid = read_u32();
				return check(EndpointQueryPacket(eid, flags));
			}

		case HXB_PTYPE_EPINFO:
			{
				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();
				return check(EndpointInfoPacket(eid, datatype, read_string(), flags));
			}

		default:
			throw BadPacketException("Unknown packet type");
	}
}

// }}}

void hexabus::deserialize(const void* packet, size_t size, PacketVisitor& handler)
{
	deserialize(packet, size)->accept(handler);
}

Packet::Ptr hexabus::deserialize(const void* packet, size_t size)
{
	return BinaryDeserializer(reinterpret_cast<const char*>(packet), size).deserialize();
}
