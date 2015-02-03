#include <libhexabus/private/serialization.hpp>

#include <algorithm>
#include <cstring>
#include <endian.h>
#include <stdexcept>

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
		uint16_t _seqNum;
		uint8_t _flags;

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

		void appendPropertyWriteValue(const PropertyWritePacket<bool>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<uint8_t>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<uint16_t>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<uint32_t>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<uint64_t>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<int8_t>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<int16_t>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<int32_t>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<int64_t>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<float>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<std::string>& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<std::array<uint8_t, 65> >& propwrite);
		void appendPropertyWriteValue(const PropertyWritePacket<std::array<uint8_t, 16> >& propwrite);

		void appendPropertyReportValue(const PropertyReportPacket<bool>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<uint8_t>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<uint16_t>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<uint32_t>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<uint64_t>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<int8_t>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<int16_t>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<int32_t>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<int64_t>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<float>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<std::string>& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<std::array<uint8_t, 65> >& propreport);
		void appendPropertyReportValue(const PropertyReportPacket<std::array<uint8_t, 16> >& propreport);

		void appendCause(const CausedPacket& packet);
		void appendOrigin(const ProxiedPacket& packet);

	public:
		BinarySerializer(std::vector<char>& target, uint16_t seqNum, uint8_t flags);

		virtual void visit(const ErrorPacket& error);
		virtual void visit(const QueryPacket& query);
		virtual void visit(const EndpointQueryPacket& endpointQuery);
		virtual void visit(const EndpointInfoPacket& endpointInfo) { appendValue(endpointInfo); }
		virtual void visit(const EndpointReportPacket& endpointReport) { appendValue(endpointReport); appendCause(endpointReport); };
		virtual void visit(const AckPacket& ack);
		virtual void visit(const PropertyQueryPacket& propertyQuery);

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

		virtual void visit(const ReportPacket<bool>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<uint8_t>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<uint16_t>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<uint32_t>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<uint64_t>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<int8_t>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<int16_t>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<int32_t>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<int64_t>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<float>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<std::string>& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<std::array<uint8_t, 65> >& report) { appendValue(report); appendCause(report); }
		virtual void visit(const ReportPacket<std::array<uint8_t, 16> >& report) { appendValue(report); appendCause(report); }

		virtual void visit(const ProxyInfoPacket<bool>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<uint8_t>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<uint16_t>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<uint32_t>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<uint64_t>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<int8_t>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<int16_t>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<int32_t>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<int64_t>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<float>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<std::string>& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<std::array<uint8_t, 65> >& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }
		virtual void visit(const ProxyInfoPacket<std::array<uint8_t, 16> >& proxyInfo) { appendValue(proxyInfo); appendOrigin(proxyInfo); }

		virtual void visit(const PropertyWritePacket<bool>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<uint8_t>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<uint16_t>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<uint32_t>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<uint64_t>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<int8_t>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<int16_t>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<int32_t>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<int64_t>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<float>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<std::string>& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<std::array<uint8_t, 65> >& propertyWrite) { appendPropertyWriteValue(propertyWrite); }
		virtual void visit(const PropertyWritePacket<std::array<uint8_t, 16> >& propertyWrite) { appendPropertyWriteValue(propertyWrite); }

		virtual void visit(const PropertyReportPacket<bool>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<uint8_t>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<uint16_t>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<uint32_t>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<uint64_t>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<int8_t>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<int16_t>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<int32_t>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<int64_t>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<float>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<std::string>& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<std::array<uint8_t, 65> >& propertyReport) { appendPropertyReportValue(propertyReport); }
		virtual void visit(const PropertyReportPacket<std::array<uint8_t, 16> >& propertyReport) { appendPropertyReportValue(propertyReport); }
};

BinarySerializer::BinarySerializer(std::vector<char>& target, uint16_t seqNum, uint8_t flags)
	: _target(target), _seqNum(seqNum), _flags(flags)
{
}

void BinarySerializer::appendHeader(const Packet& packet)
{
	_headerStart = _target.size();

	_target.insert(_target.end(), HXB_HEADER, HXB_HEADER + strlen(HXB_HEADER));
	_target.push_back(packet.type());
	_target.push_back(_flags);
	append_u16(_seqNum);
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

void BinarySerializer::appendCause(const CausedPacket& packet)
{
	append_u16(packet.cause());
}

void BinarySerializer::appendOrigin(const ProxiedPacket& packet)
{
	auto bytes = packet.origin().to_bytes();
	_target.insert(_target.end(), bytes.begin(), bytes.end());
}

void BinarySerializer::visit(const ErrorPacket& error)
{
	appendHeader(error);
	append_u8(error.code());
	appendCause(error);
}

void BinarySerializer::visit(const AckPacket& ack)
{
	appendHeader(ack);
	appendCause(ack);
}

void BinarySerializer::visit(const QueryPacket& query)
{
	appendEIDHeader(query);
}

void BinarySerializer::visit(const EndpointQueryPacket& endpointQuery)
{
	appendEIDHeader(endpointQuery);
}

void BinarySerializer::visit(const PropertyQueryPacket& propertyQuery)
{
	appendEIDHeader(propertyQuery);
	append_u32(propertyQuery.propid());
}

#define BS_APPEND_VALUE(TYPE, APPEND) \
	void BinarySerializer::appendValue(const ValuePacket<TYPE>& value) \
	{ \
		appendValueHeader(value); \
		APPEND(value.value()); \
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
}

void BinarySerializer::appendValue(const ValuePacket<std::array<uint8_t, 16> >& value)
{
	appendValueHeader(value);

	_target.resize(_target.size() + value.value().size());
	std::memcpy(&_target[_target.size() - value.value().size()], &value.value()[0], value.value().size());

}

void BinarySerializer::appendValue(const ValuePacket<std::array<uint8_t, 65> >& value)
{
	appendValueHeader(value);


	_target.resize(_target.size() + value.value().size());
	std::memcpy(&_target[_target.size() - value.value().size()], &value.value()[0], value.value().size());
}

#define ARRAY(SIZE) std::array<uint8_t, SIZE>
#define BS_APPEND_PROPERTY_WRITE_VALUE(TYPE) \
void BinarySerializer::appendPropertyWriteValue(const PropertyWritePacket<TYPE>& propwrite) \
{ \
	appendValueHeader(propwrite); \
	append_u32(propwrite.propid()); \
	appendValue(propwrite); \
}

BS_APPEND_PROPERTY_WRITE_VALUE(bool)
BS_APPEND_PROPERTY_WRITE_VALUE(uint8_t)
BS_APPEND_PROPERTY_WRITE_VALUE(uint16_t)
BS_APPEND_PROPERTY_WRITE_VALUE(uint32_t)
BS_APPEND_PROPERTY_WRITE_VALUE(uint64_t)
BS_APPEND_PROPERTY_WRITE_VALUE(int8_t)
BS_APPEND_PROPERTY_WRITE_VALUE(int16_t)
BS_APPEND_PROPERTY_WRITE_VALUE(int32_t)
BS_APPEND_PROPERTY_WRITE_VALUE(int64_t)
BS_APPEND_PROPERTY_WRITE_VALUE(float)
BS_APPEND_PROPERTY_WRITE_VALUE(std::string)
BS_APPEND_PROPERTY_WRITE_VALUE(ARRAY(16))
BS_APPEND_PROPERTY_WRITE_VALUE(ARRAY(65))

#undef BS_APPEND_PROPERTY_WRITE_VALUE

#define BS_APPEND_PROPERTY_REPORT_VALUE(TYPE) \
void BinarySerializer::appendPropertyReportValue(const PropertyReportPacket<TYPE>& propreport) \
{ \
	appendValueHeader(propreport); \
	append_u32(propreport.nextid()); \
	appendValue(propreport); \
	appendCause(propreport); \
}

BS_APPEND_PROPERTY_REPORT_VALUE(bool)
BS_APPEND_PROPERTY_REPORT_VALUE(uint8_t)
BS_APPEND_PROPERTY_REPORT_VALUE(uint16_t)
BS_APPEND_PROPERTY_REPORT_VALUE(uint32_t)
BS_APPEND_PROPERTY_REPORT_VALUE(uint64_t)
BS_APPEND_PROPERTY_REPORT_VALUE(int8_t)
BS_APPEND_PROPERTY_REPORT_VALUE(int16_t)
BS_APPEND_PROPERTY_REPORT_VALUE(int32_t)
BS_APPEND_PROPERTY_REPORT_VALUE(int64_t)
BS_APPEND_PROPERTY_REPORT_VALUE(float)
BS_APPEND_PROPERTY_REPORT_VALUE(std::string)
BS_APPEND_PROPERTY_REPORT_VALUE(ARRAY(16))
BS_APPEND_PROPERTY_REPORT_VALUE(ARRAY(65))

#undef BS_APPEND_PROPERTY_REPORT_VALUE
#undef ARRAY
// }}}

std::vector<char> hexabus::serialize(const Packet& packet, uint16_t seqNum, uint8_t flags)
{
	std::vector<char> result;
	BinarySerializer serializer(result, seqNum, flags);

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
		boost::asio::ip::address_v6 read_address_v6();

		template<size_t L>
		std::array<uint8_t, L> read_bytes();
		std::string read_string();

		template<typename T>
		Packet::Ptr checkInfo(bool info, uint32_t eid, const T& value, uint8_t flags, uint16_t sequenceNumber);

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

boost::asio::ip::address_v6 BinaryDeserializer::read_address_v6()
{
	size_t length = sizeof(boost::asio::ip::address_v6::bytes_type);

	checkLength(length);

	boost::asio::ip::address_v6::bytes_type result;
	std::copy(_packet + _offset, _packet + _offset + length, result.begin());
	_offset += length;

	return boost::asio::ip::address_v6(result);
}

template<typename T>
Packet::Ptr BinaryDeserializer::checkInfo(bool info, uint32_t eid, const T& value, uint8_t flags, uint16_t sequenceNumber)
{
	if (info) {
		return check(InfoPacket<T>(eid, value, flags, sequenceNumber));
	} else {
		return check(WritePacket<T>(eid, value, flags, sequenceNumber));
	}
}

template<typename T>
Packet::Ptr BinaryDeserializer::check(const T& packet)
{
	return Packet::Ptr(new T(packet));
}

Packet::Ptr BinaryDeserializer::deserialize()
{
	readHeader();

	uint8_t type = read_u8();
	uint8_t flags = read_u8();
	uint16_t sequenceNumber = read_u16();

	switch (type) {
		case HXB_PTYPE_ERROR:
			{
				uint8_t code = read_u8();
				uint16_t cause = read_u16();
				return check(ErrorPacket(code, cause, flags, sequenceNumber));
			}

		case HXB_PTYPE_INFO:
		case HXB_PTYPE_WRITE:
			{
				bool info = type == HXB_PTYPE_INFO;
				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();

				switch (datatype) {
					case HXB_DTYPE_BOOL: return checkInfo<bool>(info, eid, read_u8(), flags, sequenceNumber);
					case HXB_DTYPE_UINT8: return checkInfo<uint8_t>(info, eid, read_u8(), flags, sequenceNumber);
					case HXB_DTYPE_UINT16: return checkInfo<uint16_t>(info, eid, read_u16(), flags, sequenceNumber);
					case HXB_DTYPE_UINT32: return checkInfo<uint32_t>(info, eid, read_u32(), flags, sequenceNumber);
					case HXB_DTYPE_UINT64: return checkInfo<uint64_t>(info, eid, read_u64(), flags, sequenceNumber);
					case HXB_DTYPE_SINT8: return checkInfo<int8_t>(info, eid, read_s8(), flags, sequenceNumber);
					case HXB_DTYPE_SINT16: return checkInfo<int16_t>(info, eid, read_s16(), flags, sequenceNumber);
					case HXB_DTYPE_SINT32: return checkInfo<int32_t>(info, eid, read_s32(), flags, sequenceNumber);
					case HXB_DTYPE_SINT64: return checkInfo<int64_t>(info, eid, read_s64(), flags, sequenceNumber);
					case HXB_DTYPE_FLOAT: return checkInfo<float>(info, eid, read_float(), flags, sequenceNumber);
					case HXB_DTYPE_128STRING: return checkInfo<std::string>(info, eid, read_string(), flags, sequenceNumber);
					case HXB_DTYPE_16BYTES: return checkInfo<std::array<uint8_t, 16> >(info, eid, read_bytes<16>(), flags, sequenceNumber);
					case HXB_DTYPE_65BYTES: return checkInfo<std::array<uint8_t, 65> >(info, eid, read_bytes<65>(), flags, sequenceNumber);

					default:
						throw BadPacketException("Invalid datatype");
				}
			}

		case HXB_PTYPE_PINFO:
			{
				#define ARRAY(SIZE) std::array<uint8_t, SIZE>
				#define BS_DESERIALIZE_PINFO_CASE(HXBTYPE, TYPE, READ) \
				{ \
					case HXBTYPE: \
						{ \
							TYPE value = READ(); \
							return check(ProxyInfoPacket<TYPE>(read_address_v6(), eid, value, flags, sequenceNumber)); \
						} \
				} \

				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();

				switch (datatype) {
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_BOOL, bool, read_u8)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_UINT8, uint8_t, read_u8)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_UINT16, uint16_t, read_u16)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_UINT32, uint32_t, read_u32)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_UINT64, uint64_t, read_u64)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_SINT8, int8_t, read_s8)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_SINT16, int16_t, read_s16)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_SINT32, int32_t, read_s32)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_SINT64, int64_t, read_s64)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_FLOAT, float, read_float)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_128STRING, std::string, read_string)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_16BYTES, ARRAY(16), read_bytes<16>)
					BS_DESERIALIZE_PINFO_CASE(HXB_DTYPE_65BYTES, ARRAY(65), read_bytes<65>)
					default:
						throw BadPacketException("Invalid datatype");
				}
				#undef ARRAY
				#undef BS_DESERIALIZE_PINFO_CASE
			}
		case HXB_PTYPE_REPORT:
			{
				#define ARRAY(SIZE) std::array<uint8_t, SIZE>
				#define BS_DESERIALIZE_REPORT_CASE(HXBTYPE, TYPE, READ) \
				{ \
					case HXBTYPE: \
						{ \
							TYPE value = READ(); \
							return check(ReportPacket<TYPE>(read_u16(), eid, value, flags, sequenceNumber)); \
						} \
				} \

				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();

				switch (datatype) {
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_BOOL, bool, read_u8)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_UINT8, uint8_t, read_u8)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_UINT16, uint16_t, read_u16)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_UINT32, uint32_t, read_u32)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_UINT64, uint64_t, read_u64)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_SINT8, int8_t, read_s8)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_SINT16, int16_t, read_s16)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_SINT32, int32_t, read_s32)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_SINT64, int64_t, read_s64)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_FLOAT, float, read_float)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_128STRING, std::string, read_string)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_16BYTES, ARRAY(16), read_bytes<16>)
					BS_DESERIALIZE_REPORT_CASE(HXB_DTYPE_65BYTES, ARRAY(65), read_bytes<65>)
					default:
						throw BadPacketException("Invalid datatype");
				}
				#undef ARRAY
				#undef BS_DESERIALIZE_REPORT_CASE
			}
		case HXB_PTYPE_EP_PROP_REPORT:
			{
				#define ARRAY(SIZE) std::array<uint8_t, SIZE>
				#define BS_DESERIALIZE_PREPORT_CASE(HXBTYPE, TYPE, READ) \
				{ \
					case HXBTYPE: \
						{ \
							TYPE value = READ(); \
							return check(PropertyReportPacket<TYPE>(nextid, read_u16(), eid, value, flags, sequenceNumber)); \
						} \
				} \

				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();
				uint32_t nextid = read_u32();

				switch (datatype) {
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_BOOL, bool, read_u8)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_UINT8, uint8_t, read_u8)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_UINT16, uint16_t, read_u16)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_UINT32, uint32_t, read_u32)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_UINT64, uint64_t, read_u64)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_SINT8, int8_t, read_s8)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_SINT16, int16_t, read_s16)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_SINT32, int32_t, read_s32)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_SINT64, int64_t, read_s64)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_FLOAT, float, read_float)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_128STRING, std::string, read_string)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_16BYTES, ARRAY(16), read_bytes<16>)
					BS_DESERIALIZE_PREPORT_CASE(HXB_DTYPE_65BYTES, ARRAY(65), read_bytes<65>)
					default:
						throw BadPacketException("Invalid datatype");
				}
				#undef ARRAY
				#undef BS_DESERIALIZE_PREPORT_CASE
			}
		case HXB_PTYPE_EP_PROP_WRITE:
			{
				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();
				uint32_t propid = read_u32();

				switch (datatype) {
					case HXB_DTYPE_BOOL: return check(PropertyWritePacket<bool>(propid, eid, read_u8(), flags, sequenceNumber));
					case HXB_DTYPE_UINT8: return check(PropertyWritePacket<uint8_t>(propid, eid, read_u8(), flags, sequenceNumber));
					case HXB_DTYPE_UINT16: return check(PropertyWritePacket<uint16_t>(propid, eid, read_u16(), flags, sequenceNumber));
					case HXB_DTYPE_UINT32: return check(PropertyWritePacket<uint32_t>(propid, eid, read_u32(), flags, sequenceNumber));
					case HXB_DTYPE_UINT64: return check(PropertyWritePacket<uint64_t>(propid, eid, read_u64(), flags, sequenceNumber));
					case HXB_DTYPE_SINT8: return check(PropertyWritePacket<int8_t>(propid, eid, read_s8(), flags, sequenceNumber));
					case HXB_DTYPE_SINT16: return check(PropertyWritePacket<int16_t>(propid, eid, read_s16(), flags, sequenceNumber));
					case HXB_DTYPE_SINT32: return check(PropertyWritePacket<int32_t>(propid, eid, read_s32(), flags, sequenceNumber));
					case HXB_DTYPE_SINT64: return check(PropertyWritePacket<int64_t>(propid, eid, read_s64(), flags, sequenceNumber));
					case HXB_DTYPE_FLOAT: return check(PropertyWritePacket<float>(propid, eid, read_float(), flags, sequenceNumber));
					case HXB_DTYPE_128STRING: return check(PropertyWritePacket<std::string>(propid, eid, read_string(), flags, sequenceNumber));
					case HXB_DTYPE_16BYTES: return check(PropertyWritePacket<std::array<uint8_t, 16> >(propid, eid, read_bytes<16>(), flags, sequenceNumber));
					case HXB_DTYPE_65BYTES: return check(PropertyWritePacket<std::array<uint8_t, 65> >(propid, eid, read_bytes<65>(), flags, sequenceNumber));

					default:
						throw BadPacketException("Invalid datatype");
				}
			}

		case HXB_PTYPE_QUERY:
			{
				uint32_t eid = read_u32();
				return check(QueryPacket(eid, flags, sequenceNumber));
			}

		case HXB_PTYPE_EPQUERY:
			{
				uint32_t eid = read_u32();
				return check(EndpointQueryPacket(eid, flags, sequenceNumber));
			}

		case HXB_PTYPE_EPINFO:
			{
				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();
				return check(EndpointInfoPacket(eid, datatype, read_string(), flags, sequenceNumber));
			}

		case HXB_PTYPE_EPREPORT:
			{
				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();
				std::string value = read_string();
				uint16_t cause = read_u16();
				return check(EndpointReportPacket(cause, eid, datatype, value, flags, sequenceNumber));
			}

		case HXB_PTYPE_ACK:
			{
				uint16_t cause = read_u16();
				return check(AckPacket(cause, flags, sequenceNumber));
			}

		case HXB_PTYPE_EP_PROP_QUERY:
			{
				uint32_t eid = read_u32();
				uint32_t propid = read_u32();
				return check(PropertyQueryPacket(propid, eid, flags, sequenceNumber));
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
