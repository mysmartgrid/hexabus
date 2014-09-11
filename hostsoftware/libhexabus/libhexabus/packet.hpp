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
#include <libhexabus/hexabus_types.h>

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
			virtual Packet* clone() const = 0;
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
	class TimeInfoPacket;
	class PropertyQueryPacket;
	template<typename TValue>
	class PropertyWritePacket;
	template<typename TValue>
	class PropertyReportPacket;

	template<template<typename TValue> class TPacket>
	class TypedPacketVisitor {
		public:
			virtual ~TypedPacketVisitor() {}

			virtual void visit(const TPacket<bool>& info) = 0;
			virtual void visit(const TPacket<uint8_t>& info) = 0;
			virtual void visit(const TPacket<uint32_t>& info) = 0;
			virtual void visit(const TPacket<float>& info) = 0;
			virtual void visit(const TPacket<std::string>& info) = 0;
			virtual void visit(const TPacket<boost::array<char, 16> >& info) = 0;
			virtual void visit(const TPacket<boost::array<char, 65> >& info) = 0;

			class Empty : public virtual TypedPacketVisitor {
				public:
					virtual void visit(const TPacket<bool>& info) {}
					virtual void visit(const TPacket<uint8_t>& info) {}
					virtual void visit(const TPacket<uint32_t>& info) {}
					virtual void visit(const TPacket<float>& info) {}
					virtual void visit(const TPacket<std::string>& info) {}
					virtual void visit(const TPacket<boost::array<char, 16> >& info) {}
					virtual void visit(const TPacket<boost::array<char, 65> >& info) {}
			};
	};

	class PacketVisitor :
			public virtual TypedPacketVisitor<InfoPacket>,
			public virtual TypedPacketVisitor<ReportPacket>,
			public virtual TypedPacketVisitor<WritePacket>,
			public virtual TypedPacketVisitor<ProxyInfoPacket>,
			public virtual TypedPacketVisitor<PropertyWritePacket>,
			public virtual TypedPacketVisitor<PropertyReportPacket> {
		public:
			virtual ~PacketVisitor() {}

			using TypedPacketVisitor<InfoPacket>::visit;
			using TypedPacketVisitor<ReportPacket>::visit;
			using TypedPacketVisitor<WritePacket>::visit;
			using TypedPacketVisitor<ProxyInfoPacket>::visit;
			using TypedPacketVisitor<PropertyWritePacket>::visit;
			using TypedPacketVisitor<PropertyReportPacket>::visit;

			virtual void visit(const ErrorPacket& error) = 0;
			virtual void visit(const QueryPacket& query) = 0;
			virtual void visit(const EndpointQueryPacket& endpointQuery) = 0;
			virtual void visit(const EndpointInfoPacket& endpointInfo) = 0;

			virtual void visit(const EndpointReportPacket& endpointReport) = 0;
			virtual void visit(const AckPacket& ack) = 0;
			virtual void visit(const TimeInfoPacket& timeinfo) = 0;
			virtual void visit(const PropertyQueryPacket& propertyQuery) = 0;

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
			virtual ~CausedPacket() {}

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

			Packet* clone() const {
				return new ErrorPacket(*this);
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

			Packet* clone() const {
				return new QueryPacket(*this);
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

			Packet* clone() const {
				return new EndpointQueryPacket(*this);
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

					|| boost::is_same<TValue, boost::array<char, 16> >::value
					|| boost::is_same<TValue, boost::array<char, 65> >::value),
				"I don't know how to handle that type");

			static uint8_t calculateDatatype()
			{
				return boost::is_same<TValue, bool>::value ? HXB_DTYPE_BOOL :
					boost::is_same<TValue, uint8_t>::value ? HXB_DTYPE_UINT8 :
					boost::is_same<TValue, uint32_t>::value ? HXB_DTYPE_UINT32 :
					boost::is_same<TValue, float>::value ? HXB_DTYPE_FLOAT :
					boost::is_same<TValue, boost::array<char, 16> >::value
						? HXB_DTYPE_16BYTES :
					boost::is_same<TValue, boost::array<char, 65> >::value
						? HXB_DTYPE_65BYTES :
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
				if (value.size() > max_length)
					throw std::out_of_range("value");
			}

			ValuePacket(uint8_t type, uint32_t eid, uint8_t datatype, const std::string& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: TypedPacket(type, eid, datatype, flags, sequenceNumber), _value(value)
			{
				if (value.size() > max_length)
					throw std::out_of_range("value");
			}

		public:
			const std::string& value() const { return _value; }
			static const unsigned int max_length = 127;
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

			Packet* clone() const {
				return new InfoPacket(*this);
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

			Packet* clone() const {
				return new ReportPacket(*this);
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

			Packet* clone() const {
				return new ProxyInfoPacket(*this);
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

			Packet* clone() const {
				return new WritePacket(*this);
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

			Packet* clone() const {
				return new EndpointInfoPacket(*this);
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

			Packet* clone() const {
				return new EndpointReportPacket(*this);
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

			Packet* clone() const {
				return new AckPacket(*this);
			}
	};

	class TimeInfoPacket: public Packet {
		private:
			boost::posix_time::ptime _datetime;

		public:
			TimeInfoPacket(boost::posix_time::ptime datetime, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: Packet(HXB_PTYPE_TIMEINFO, flags, sequenceNumber), _datetime(datetime)
				{}

			boost::posix_time::ptime datetime() const { return _datetime; }

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}

			Packet* clone() const {
				return new TimeInfoPacket(*this);
			}
		};

	class PropertyQueryPacket : public EIDPacket {
		private:
			uint32_t _propid;

		public:
			PropertyQueryPacket(uint32_t propid, uint32_t eid, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: EIDPacket(HXB_PTYPE_EP_PROP_QUERY, eid, flags, sequenceNumber), _propid(propid)
			{}

			uint32_t propid() const { return _propid; }

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}

			Packet* clone() const {
				return new PropertyQueryPacket(*this);
			}
	};

	template<typename TValue>
	class PropertyWritePacket : public ValuePacket<TValue> {
		private:
			uint32_t _propid;

		public:
			PropertyWritePacket(uint32_t propid, uint32_t eid, const TValue& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: ValuePacket<TValue>(HXB_PTYPE_EP_PROP_WRITE, eid, value, flags, sequenceNumber), _propid(propid)
			{}

			uint32_t propid() const { return _propid; }

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}

			Packet* clone() const {
				return new PropertyWritePacket(*this);
			}
	};

	template<typename TValue>
	class PropertyReportPacket : public ValuePacket<TValue>, public CausedPacket {
		private:
			uint32_t _nextid;
		public:
			PropertyReportPacket(uint32_t nextid, int16_t cause, uint32_t eid, const TValue& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: ValuePacket<TValue>(HXB_PTYPE_REPORT, eid, value, flags, sequenceNumber), CausedPacket(cause), _nextid(nextid)
			{}

			uint32_t nextid() const { return _nextid; }

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}

			Packet* clone() const {
				return new PropertyReportPacket(*this);
			}
	};

};

#endif
