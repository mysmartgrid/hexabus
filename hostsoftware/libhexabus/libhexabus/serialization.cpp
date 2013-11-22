#include <libhexabus/private/serialization.hpp>

#include <stdexcept>
#include <algorithm>
#include <arpa/inet.h>

#include "crc.hpp"
#include "error.hpp"

using namespace hexabus;

// {{{ Binary serialization visitor

// {{{ Buffer with common functions

class SerializerBuffer {
	protected:
		std::vector<char> _target;

	public:
		void append_u8(uint8_t value);
		void append_u16(uint16_t value);
		void append_u32(uint32_t value);
		void append_float(float value);

		void appendValue(const ValuePacket<bool>& value);
		void appendValue(const ValuePacket<uint8_t>& value);
		void appendValue(const ValuePacket<uint32_t>& value);
		void appendValue(const ValuePacket<float>& value);
		void appendValue(const ValuePacket<boost::posix_time::ptime>& value);
		void appendValue(const ValuePacket<boost::posix_time::time_duration>& value);
		void appendValue(const ValuePacket<std::string>& value);
		void appendValue(const ValuePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& value);
		void appendValue(const ValuePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& value);

		void append_crc();

		std::vector<char>& buffer() { return _target; }
};

void SerializerBuffer::append_u8(uint8_t value)
{
	_target.push_back(value);
}

void SerializerBuffer::append_u16(uint16_t value)
{
	union {
		uint16_t u16;
		char raw[sizeof(value)];
	} c;

	c.u16 = htons(value);

	_target.insert(_target.end(), c.raw, c.raw + sizeof(c.raw));
}

void SerializerBuffer::append_u32(uint32_t value)
{
	union {
		uint32_t u32;
		char raw[sizeof(value)];
	} c = { htonl(value) };

	_target.insert(_target.end(), c.raw, c.raw + sizeof(c.raw));
}

void SerializerBuffer::append_float(float value)
{
	union {
		float f;
		uint32_t u32;
		char raw[sizeof(value)];
	} c = { value };
	c.u32 = htonl(c.u32);

	_target.insert(_target.end(), c.raw, c.raw + sizeof(c.raw));
}

void SerializerBuffer::appendValue(const ValuePacket<bool>& value)
{
	append_u8(value.value());
}

void SerializerBuffer::appendValue(const ValuePacket<uint8_t>& value)
{
	append_u8(value.value());
}

void SerializerBuffer::appendValue(const ValuePacket<uint32_t>& value)
{
	append_u32(value.value());
}

void SerializerBuffer::appendValue(const ValuePacket<float>& value)
{
	append_float(value.value());
}

void SerializerBuffer::appendValue(const ValuePacket<boost::posix_time::ptime>& value)
{
	append_u8(value.value().time_of_day().hours());
	append_u8(value.value().time_of_day().minutes());
	append_u8(value.value().time_of_day().seconds());
	append_u8(value.value().date().day());
	append_u8(value.value().date().month());
	append_u16(value.value().date().year());
	append_u8(value.value().date().day_of_week());
}

void SerializerBuffer::appendValue(const ValuePacket<boost::posix_time::time_duration>& value)
{
	append_u32(value.value().total_seconds());
}

void SerializerBuffer::appendValue(const ValuePacket<std::string>& value)
{
	_target.insert(_target.end(), value.value().begin(), value.value().end());
	_target.insert(_target.end(), HXB_STRING_PACKET_MAX_BUFFER_LENGTH + 1 - value.value().size(), '\0');
}

void SerializerBuffer::appendValue(const ValuePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& value)
{
	_target.insert(_target.end(), value.value().begin(), value.value().end());
}

void SerializerBuffer::appendValue(const ValuePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& value)
{
	_target.insert(_target.end(), value.value().begin(), value.value().end());
}

void SerializerBuffer::append_crc()
{
	uint16_t crc = hexabus::crc(&_target[0], _target.size());

	append_u16(crc);
}

// }}}

template<template<typename TValue> class TPacket>
class TypedPacketSerializer: public virtual SerializerBuffer, public virtual TypedPacketVisitor<TPacket> {
	private:
		template<typename TValue>
		void append(const TPacket<TValue>& packet)
		{
			append_u32(packet.eid());
			append_u8(packet.datatype());
			appendValue(packet);
		}

		virtual void visit(const TPacket<bool>& packet) { append(packet); }
		virtual void visit(const TPacket<uint8_t>& packet) { append(packet); }
		virtual void visit(const TPacket<uint32_t>& packet) { append(packet); }
		virtual void visit(const TPacket<float>& packet) { append(packet); }
		virtual void visit(const TPacket<boost::posix_time::ptime>& packet) { append(packet); }
		virtual void visit(const TPacket<boost::posix_time::time_duration>& packet) { append(packet); }
		virtual void visit(const TPacket<std::string>& packet) { append(packet); }
		virtual void visit(const TPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& packet) { append(packet); }
		virtual void visit(const TPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& packet) { append(packet); }
};

class ReportPacketSerializer: public virtual SerializerBuffer, public virtual TypedPacketVisitor<ReportPacket> {
	private:
		template<typename TValue>
		void append(const ReportPacket<TValue>& packet)
		{
			append_u16(packet.cause());
			append_u32(packet.eid());
			append_u8(packet.datatype());
			appendValue(packet);
		}

		virtual void visit(const ReportPacket<bool>& packet) { append(packet); }
		virtual void visit(const ReportPacket<uint8_t>& packet) { append(packet); }
		virtual void visit(const ReportPacket<uint32_t>& packet) { append(packet); }
		virtual void visit(const ReportPacket<float>& packet) { append(packet); }
		virtual void visit(const ReportPacket<boost::posix_time::ptime>& packet) { append(packet); }
		virtual void visit(const ReportPacket<boost::posix_time::time_duration>& packet) { append(packet); }
		virtual void visit(const ReportPacket<std::string>& packet) { append(packet); }
		virtual void visit(const ReportPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& packet) { append(packet); }
		virtual void visit(const ReportPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& packet) { append(packet); }
};

class ProxyInfoPacketSerializer : public virtual SerializerBuffer, public virtual TypedPacketSerializer<ProxyInfoPacket> {
	private:
		template<typename TValue>
		void append(const ProxyInfoPacket<TValue>& packet)
		{
			boost::asio::ip::address_v6::bytes_type source = packet.source().to_bytes();

			_target.insert(_target.end(), source.begin(), source.end());
			append_u32(packet.eid());
			append_u8(packet.datatype());
			appendValue(packet);
		}

		virtual void visit(const ProxyInfoPacket<bool>& packet) { append(packet); }
		virtual void visit(const ProxyInfoPacket<uint8_t>& packet) { append(packet); }
		virtual void visit(const ProxyInfoPacket<uint32_t>& packet) { append(packet); }
		virtual void visit(const ProxyInfoPacket<float>& packet) { append(packet); }
		virtual void visit(const ProxyInfoPacket<boost::posix_time::ptime>& packet) { append(packet); }
		virtual void visit(const ProxyInfoPacket<boost::posix_time::time_duration>& packet) { append(packet); }
		virtual void visit(const ProxyInfoPacket<std::string>& packet) { append(packet); }
		virtual void visit(const ProxyInfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& packet) { append(packet); }
		virtual void visit(const ProxyInfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& packet) { append(packet); }
};

class BinarySerializer :
		private virtual SerializerBuffer,
		private virtual TypedPacketSerializer<InfoPacket>,
		private virtual ReportPacketSerializer,
		private virtual TypedPacketSerializer<WritePacket>,
		private virtual ProxyInfoPacketSerializer,
		private virtual PacketVisitor {
	public:
		std::vector<char> serialize(const Packet& packet, uint16_t seqNum);

	private:
		virtual void visit(const ErrorPacket& error);
		virtual void visit(const QueryPacket& query);
		virtual void visit(const EndpointQueryPacket& endpointQuery);
		virtual void visit(const EndpointInfoPacket& endpointInfo);
		virtual void visit(const EndpointReportPacket& endpointReport);
		virtual void visit(const AckPacket& ack);
};

std::vector<char> BinarySerializer::serialize(const Packet& packet, uint16_t seqNum)
{
	_target.clear();

	_target.insert(_target.end(), HXB_HEADER, HXB_HEADER + strlen(HXB_HEADER));
	append_u8(packet.type());
	append_u8(packet.flags());
	append_u16(seqNum);
	packet.accept(*this);
	append_crc();

	return _target;
}

void BinarySerializer::visit(const ErrorPacket& error)
{
	append_u8(error.code());
	append_u16(error.cause());
}

void BinarySerializer::visit(const QueryPacket& query)
{
	append_u32(query.eid());
}

void BinarySerializer::visit(const EndpointQueryPacket& endpointQuery)
{
	append_u32(endpointQuery.eid());
}

void BinarySerializer::visit(const EndpointInfoPacket& endpointInfo)
{
	append_u32(endpointInfo.eid());
	append_u8(endpointInfo.datatype());
	appendValue(endpointInfo);
}

void BinarySerializer::visit(const EndpointReportPacket& endpointReport)
{
	append_u16(endpointReport.cause());
	append_u32(endpointReport.eid());
	append_u8(endpointReport.datatype());
	appendValue(endpointReport);
}

void BinarySerializer::visit(const AckPacket& ack)
{
	append_u16(ack.cause());
}

// }}}

std::vector<char> hexabus::serialize(const Packet& packet, uint16_t seqNum)
{
	BinarySerializer serializer;

	return serializer.serialize(packet, seqNum);
}

// {{{ Binary deserialization

class BinaryDeserializer {
	private:
		const char* _packet;
		size_t _offset;
		size_t _size;

		void checkLength(size_t min);

		void readHeader();

		uint8_t read_u8();
		uint16_t read_u16();
		uint32_t read_u32();
		float read_float();
		template<size_t L>
		boost::array<char, L> read_bytes();
		std::string read_string();

		template<typename T>
		Packet::Ptr checkInfo(uint8_t packetType, uint16_t cause, uint32_t eid, const T& value, uint8_t flags, uint16_t seqNum);

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

uint8_t BinaryDeserializer::read_u8()
{
	checkLength(sizeof(uint8_t));

	return *(_packet + _offset++);
}

uint16_t BinaryDeserializer::read_u16()
{
	checkLength(sizeof(uint16_t));

	union {
		char raw[sizeof(uint16_t)];
		uint16_t u16;
	} c;
	memcpy(c.raw, _packet + _offset, sizeof(c.raw));

	_offset += sizeof(uint16_t);
	return ntohs(c.u16);
}

uint32_t BinaryDeserializer::read_u32()
{
	checkLength(sizeof(uint32_t));

	union C {
		char raw[sizeof(uint32_t)];
		uint32_t u32;
	} c;
	memcpy(c.raw, _packet + _offset, sizeof(c.raw));

	_offset += sizeof(uint32_t);
	return ntohl(c.u32);
}

float BinaryDeserializer::read_float()
{
	union {
		uint32_t u32;
		float f;
	} c = { read_u32() };

	return c.f;
}

template<size_t L>
boost::array<char, L> BinaryDeserializer::read_bytes()
{
	checkLength(L);

	boost::array<char, L> result;
	std::copy(_packet + _offset, _packet + _offset + L, result.begin());
	_offset += L;

	return result;
}

std::string BinaryDeserializer::read_string()
{
	checkLength(HXB_STRING_PACKET_MAX_BUFFER_LENGTH);

	if (!std::find(_packet + _offset, _packet + _offset + HXB_STRING_PACKET_MAX_BUFFER_LENGTH, '\0'))
		throw BadPacketException("Unterminated string");

	std::string result(_packet + _offset);
	_offset += HXB_STRING_PACKET_MAX_BUFFER_LENGTH + 1;

	return result;
}

template<typename T>
Packet::Ptr BinaryDeserializer::checkInfo(uint8_t packetType, uint16_t cause, uint32_t eid, const T& value, uint8_t flags, uint16_t seqNum)
{
	switch (packetType) {
		case HXB_PTYPE_INFO:
			return check(InfoPacket<T>(eid, value, flags, seqNum));

		case HXB_PTYPE_WRITE:
			return check(WritePacket<T>(eid, value, flags, seqNum));

		case HXB_PTYPE_REPORT:
			return check(ReportPacket<T>(cause, eid, value, flags, seqNum));

		default:
			throw BadPacketException("checkInfo assumptions violated");
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
	uint16_t seqNum = read_u16();

	switch (type) {
		case HXB_PTYPE_ERROR:
			{
				uint8_t code = read_u8();
				uint16_t cause = read_u16();
				return check(ErrorPacket(code, cause, flags, seqNum));
			}

		case HXB_PTYPE_INFO:
		case HXB_PTYPE_WRITE:
		case HXB_PTYPE_REPORT:
			{
				uint16_t cause = 0;
				if (type == HXB_PTYPE_REPORT) {
					cause = read_u16();
				}
				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();

				switch (datatype) {
					case HXB_DTYPE_BOOL:
						return checkInfo<bool>(type, cause, eid, read_u8(), flags, seqNum);

					case HXB_DTYPE_UINT8:
						return checkInfo<uint8_t>(type, cause, eid, read_u8(), flags, seqNum);

					case HXB_DTYPE_UINT32:
						return checkInfo<uint32_t>(type, cause, eid, read_u32(), flags, seqNum);

					case HXB_DTYPE_FLOAT:
						return checkInfo<float>(type, cause, eid, read_float(), flags, seqNum);

					case HXB_DTYPE_DATETIME:
						{
							uint8_t hour = read_u8();
							uint8_t minute = read_u8();
							uint8_t second = read_u8();
							uint8_t day = read_u8();
							uint8_t month = read_u8();
							uint16_t year = read_u8();
							uint8_t weekday = read_u8();

							boost::posix_time::ptime dt(
								boost::gregorian::date(year, month, day),
								boost::posix_time::hours(hour)
									+ boost::posix_time::minutes(minute)
									+ boost::posix_time::seconds(second));

							if (dt.date().day_of_week() != weekday)
								throw BadPacketException("Invalid datetime format");

							return checkInfo<boost::posix_time::ptime>(type, cause, eid, dt, flags, seqNum);
						}

					case HXB_DTYPE_TIMESTAMP:
						return checkInfo<boost::posix_time::time_duration>(type, cause, eid, boost::posix_time::seconds(read_u32()), flags, seqNum);

					case HXB_DTYPE_128STRING:
						return checkInfo<std::string>(type, cause, eid, read_string(), flags, seqNum);

					case HXB_DTYPE_16BYTES:
						return checkInfo<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >(type, cause, eid, read_bytes<HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH>(), flags, seqNum);

					case HXB_DTYPE_66BYTES:
						return checkInfo<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >(type, cause, eid, read_bytes<HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH>(), flags, seqNum);

					default:
						throw BadPacketException("Invalid datatype");
				}
			}

		case HXB_PTYPE_QUERY:
			{
				uint32_t eid = read_u32();
				return check(QueryPacket(eid, flags, seqNum));
			}

		case HXB_PTYPE_EPQUERY:
			{
				uint32_t eid = read_u32();
				return check(EndpointQueryPacket(eid, flags, seqNum));
			}

		case HXB_PTYPE_EPINFO:
			{
				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();
				return check(EndpointInfoPacket(eid, datatype, read_string(), flags, seqNum));
			}

		case HXB_PTYPE_EPREPORT:
			{
				uint16_t cause = read_u16();
				uint32_t eid = read_u32();
				uint8_t datatype = read_u8();
				return check(EndpointReportPacket(cause, eid, datatype, read_string(), flags, seqNum));
			}

		case HXB_PTYPE_ACK:
			{
				return check(AckPacket(read_u16(), flags, seqNum));
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
