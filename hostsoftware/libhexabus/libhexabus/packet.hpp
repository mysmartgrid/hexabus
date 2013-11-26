#ifndef LIBHEXABUS_PACKET_HPP
#define LIBHEXABUS_PACKET_HPP 1

#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include <boost/static_assert.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/type_traits.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/array.hpp>
#include "../../../shared/hexabus_types.h"
#include "../../../shared/hexabus_definitions.h"

//#define ENABLE_LOGGING 0
//#include "config.h"
#include <libhexabus/config.h>

/* Include TR1 shared ptrs in a portable way. */
#include <cstddef> // for __GLIBCXX__
#ifdef __GLIBCXX__
#  include <tr1/memory>
#else
#  ifdef __IBMCPP__
#    define __IBMCPP_TR1__
#  endif
#  include <memory>
#endif

namespace hexabus {
	class PacketVisitor;

	class Packet {
		public:
			typedef std::tr1::shared_ptr<Packet> Ptr;

			enum Flags {
				want_ack = HXB_FLAG_WANT_ACK
			};

		private:
			uint8_t _type;
			uint8_t _flags;
			uint16_t _sequenceNumber;

		protected:
			Packet(uint8_t type, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: _type(type), _flags(flags), _sequenceNumber(sequenceNumber)
			{}

		public:
			virtual ~Packet() {}

			uint8_t type() const { return _type; }
			uint8_t flags() const { return _flags; }
			uint16_t sequenceNumber() const { return _sequenceNumber; }

			virtual void accept(PacketVisitor& visitor) const = 0;
	};

	class ErrorPacket;
	class QueryPacket;
	class EndpointQueryPacket;
	template<typename TValue>
	class InfoPacket;
	template<typename TValue>
	class ReportPacket;
	template<typename TValue>
	class WritePacket;
	class EndpointInfoPacket;
	class EndpointReportPacket;
	class AckPacket;
	template<typename TValue>
	class ProxyInfoPacket;

	template<template<typename TValue> class TPacket>
	class TypedPacketVisitor {
		public:
			virtual ~TypedPacketVisitor() {}

			virtual void visit(const TPacket<bool>& info) = 0;
			virtual void visit(const TPacket<uint8_t>& info) = 0;
			virtual void visit(const TPacket<uint32_t>& info) = 0;
			virtual void visit(const TPacket<float>& info) = 0;
			virtual void visit(const TPacket<boost::posix_time::ptime>& info) = 0;
			virtual void visit(const TPacket<boost::posix_time::time_duration>& info) = 0;
			virtual void visit(const TPacket<std::string>& info) = 0;
			virtual void visit(const TPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) = 0;
			virtual void visit(const TPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) = 0;

			class Empty : public virtual TypedPacketVisitor {
				public:
					virtual void visit(const TPacket<bool>& info) {}
					virtual void visit(const TPacket<uint8_t>& info) {}
					virtual void visit(const TPacket<uint32_t>& info) {}
					virtual void visit(const TPacket<float>& info) {}
					virtual void visit(const TPacket<boost::posix_time::ptime>& info) {}
					virtual void visit(const TPacket<boost::posix_time::time_duration>& info) {}
					virtual void visit(const TPacket<std::string>& info) {}
					virtual void visit(const TPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {}
					virtual void visit(const TPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {}
			};
	};

	class PacketVisitor :
			public virtual TypedPacketVisitor<InfoPacket>,
			public virtual TypedPacketVisitor<ReportPacket>,
			public virtual TypedPacketVisitor<WritePacket>,
			public virtual TypedPacketVisitor<ProxyInfoPacket> {
		public:
			virtual ~PacketVisitor() {}

			using TypedPacketVisitor<InfoPacket>::visit;
			using TypedPacketVisitor<ReportPacket>::visit;
			using TypedPacketVisitor<WritePacket>::visit;
			using TypedPacketVisitor<ProxyInfoPacket>::visit;

			virtual void visit(const ErrorPacket& error) = 0;
			virtual void visit(const QueryPacket& query) = 0;
			virtual void visit(const EndpointQueryPacket& endpointQuery) = 0;
			virtual void visit(const EndpointInfoPacket& endpointInfo) = 0;
			virtual void visit(const EndpointReportPacket& endpointReport) = 0;
			virtual void visit(const AckPacket& ack) = 0;

			virtual void visitPacket(const Packet& packet)
			{
				packet.accept(*this);
			}
	};

	class CausedPacket {
		private:
			uint16_t _cause;

		protected:
			CausedPacket(uint16_t cause) : _cause(cause) {}
			~CausedPacket() {}

		public:
			uint16_t cause() const { return _cause; }
	};

	class ErrorPacket : public Packet, public CausedPacket {
		private:
			uint8_t _code;

		public:
			ErrorPacket(uint8_t code, uint16_t cause, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: Packet(HXB_PTYPE_ERROR, flags, sequenceNumber), CausedPacket(cause), _code(code)
			{}

			uint8_t code() const { return _code; }

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	class EIDPacket : public Packet {
		private:
			uint32_t _eid;

		public:
			EIDPacket(uint8_t type, uint32_t eid, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: Packet(type, flags, sequenceNumber), _eid(eid)
			{}

			uint32_t eid() const { return _eid; }
	};

	class QueryPacket : public EIDPacket {
		public:
			QueryPacket(uint32_t eid, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: EIDPacket(HXB_PTYPE_QUERY, eid, flags, sequenceNumber)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	class EndpointQueryPacket : public EIDPacket {
		public:
			EndpointQueryPacket(uint32_t eid, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: EIDPacket(HXB_PTYPE_EPQUERY, eid, flags, sequenceNumber)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	class TypedPacket : public EIDPacket {
		private:
			uint8_t _datatype;

		public:
			TypedPacket(uint8_t type, uint32_t eid, uint8_t datatype, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: EIDPacket(type, eid, flags, sequenceNumber), _datatype(datatype)
			{}

			uint8_t datatype() const { return _datatype; }
	};

	template<typename TValue>
	class ValuePacket : public TypedPacket {
		private:
			BOOST_STATIC_ASSERT_MSG((
				boost::is_same<TValue, bool>::value
					|| boost::is_same<TValue, uint8_t>::value
					|| boost::is_same<TValue, uint32_t>::value
					|| boost::is_same<TValue, float>::value
					|| boost::is_same<TValue, boost::posix_time::ptime>::value
					|| boost::is_same<TValue, boost::posix_time::time_duration>::value
					|| boost::is_same<TValue, boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >::value
					|| boost::is_same<TValue, boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >::value),
				"I don't know how to handle that type");

			static uint8_t calculateDatatype()
			{
				return boost::is_same<TValue, bool>::value ? HXB_DTYPE_BOOL :
					boost::is_same<TValue, uint8_t>::value ? HXB_DTYPE_UINT8 :
					boost::is_same<TValue, uint32_t>::value ? HXB_DTYPE_UINT32 :
					boost::is_same<TValue, float>::value ? HXB_DTYPE_FLOAT :
					boost::is_same<TValue, boost::posix_time::ptime>::value ? HXB_DTYPE_DATETIME :
					boost::is_same<TValue, boost::posix_time::time_duration>::value ? HXB_DTYPE_TIMESTAMP :
					boost::is_same<TValue, boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >::value
						? HXB_DTYPE_16BYTES :
					boost::is_same<TValue, boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >::value
						? HXB_DTYPE_66BYTES :
					(throw "BUG: Unknown datatype!", HXB_DTYPE_UNDEFINED);
			}

			TValue _value;

		protected:
			ValuePacket(uint8_t type, uint32_t eid, const TValue& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: TypedPacket(type, eid, calculateDatatype(), flags, sequenceNumber), _value(value)
			{}

		public:
			const TValue& value() const { return _value; }
	};

	template<>
	class ValuePacket<std::string> : public TypedPacket {
		private:
			std::string _value;

		protected:
			ValuePacket(uint8_t type, uint32_t eid, const std::string& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: TypedPacket(type, eid, HXB_DTYPE_128STRING, flags, sequenceNumber), _value(value)
			{
				if (value.size() > HXB_STRING_PACKET_MAX_BUFFER_LENGTH)
					throw std::out_of_range("value");
			}

			ValuePacket(uint8_t type, uint32_t eid, uint8_t datatype, const std::string& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: TypedPacket(type, eid, datatype, flags, sequenceNumber), _value(value)
			{
				if (value.size() > HXB_STRING_PACKET_MAX_BUFFER_LENGTH)
					throw std::out_of_range("value");
			}

		public:
			const std::string& value() const { return _value; }
	};

	template<typename TValue>
	class InfoPacket : public ValuePacket<TValue> {
		public:
			InfoPacket(uint32_t eid, const TValue& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: ValuePacket<TValue>(HXB_PTYPE_INFO, eid, value, flags, sequenceNumber)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	template<typename TValue>
	class ReportPacket : public ValuePacket<TValue>, public CausedPacket {
		public:
			ReportPacket(uint16_t cause, uint32_t eid, const TValue& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: ValuePacket<TValue>(HXB_PTYPE_REPORT, eid, value, flags, sequenceNumber), CausedPacket(cause)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	class ProxiedPacket {
		private:
			boost::asio::ip::address_v6 _origin;

		protected:
			ProxiedPacket(const boost::asio::ip::address_v6& origin) : _origin(origin) {}
			~ProxiedPacket() {}

		public:
			const boost::asio::ip::address_v6& origin() const { return _origin; }
	};

	template<typename TValue>
	class ProxyInfoPacket : public ValuePacket<TValue>, public ProxiedPacket {
		public:
			ProxyInfoPacket(const boost::asio::ip::address_v6& origin, uint32_t eid, const TValue& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: ValuePacket<TValue>(HXB_PTYPE_PINFO, eid, value, flags, sequenceNumber), ProxiedPacket(origin)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	template<typename TValue>
	class WritePacket : public ValuePacket<TValue> {
		public:
			WritePacket(uint32_t eid, const TValue& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: ValuePacket<TValue>(HXB_PTYPE_WRITE, eid, value, flags, sequenceNumber)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	class EndpointInfoPacket : public ValuePacket<std::string> {
		public:
			EndpointInfoPacket(uint32_t eid, uint8_t datatype, const std::string& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: ValuePacket<std::string>(HXB_PTYPE_EPINFO, eid, datatype, value, flags, sequenceNumber)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	class EndpointReportPacket : public ValuePacket<std::string>, public CausedPacket {
		public:
			EndpointReportPacket(uint16_t cause, uint32_t eid, uint8_t datatype, const std::string& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: ValuePacket<std::string>(HXB_PTYPE_EPREPORT, eid, datatype, value, flags, sequenceNumber), CausedPacket(cause)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	class AckPacket : public Packet, public CausedPacket {
		public:
			AckPacket(uint16_t cause, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: Packet(HXB_PTYPE_ACK, flags, sequenceNumber), CausedPacket(cause)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};
};

#endif
