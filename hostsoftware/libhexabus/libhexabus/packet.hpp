#ifndef LIBHEXABUS_PACKET_HPP
#define LIBHEXABUS_PACKET_HPP 1

#include <stdint.h>
#include <iostream>
#include <string>
#include <vector>
#include <boost/static_assert.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/type_traits.hpp>
#include "../../../shared/hexabus_packet.h"

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
	class ErrorPacket;
	class QueryPacket;
	class EndpointQueryPacket;
	template<typename TValue>
	class InfoPacket;
	template<typename TValue>
	class WritePacket;
	class EndpointInfoPacket;

	class PacketVisitor {
		public:
			virtual ~PacketVisitor() {}

			virtual void visit(const ErrorPacket& error) = 0;
			virtual void visit(const QueryPacket& query) = 0;
			virtual void visit(const EndpointQueryPacket& endpointQuery) = 0;
			virtual void visit(const EndpointInfoPacket& endpointInfo) = 0;

			virtual void visit(const InfoPacket<bool>& info) = 0;
			virtual void visit(const InfoPacket<uint8_t>& info) = 0;
			virtual void visit(const InfoPacket<uint32_t>& info) = 0;
			virtual void visit(const InfoPacket<float>& info) = 0;
			virtual void visit(const InfoPacket<boost::posix_time::ptime>& info) = 0;
			virtual void visit(const InfoPacket<boost::posix_time::time_duration>& info) = 0;
			virtual void visit(const InfoPacket<std::string>& info) = 0;
			virtual void visit(const InfoPacket<std::vector<char> >& info) = 0;

			virtual void visit(const WritePacket<bool>& write) = 0;
			virtual void visit(const WritePacket<uint8_t>& write) = 0;
			virtual void visit(const WritePacket<uint32_t>& write) = 0;
			virtual void visit(const WritePacket<float>& write) = 0;
			virtual void visit(const WritePacket<boost::posix_time::ptime>& write) = 0;
			virtual void visit(const WritePacket<boost::posix_time::time_duration>& write) = 0;
			virtual void visit(const WritePacket<std::string>& write) = 0;
			virtual void visit(const WritePacket<std::vector<char> >& write) = 0;
	};

	class Packet {
		public:
			typedef std::tr1::shared_ptr<Packet> Ptr;

		private:
			uint8_t _type;
			uint8_t _flags;

		protected:
			Packet(uint8_t type, uint8_t flags = 0)
				: _type(type), _flags(flags)
			{}

		public:
			virtual ~Packet() {}

			uint8_t type() const { return _type; }
			uint8_t flags() const { return _flags; }

			virtual void accept(PacketVisitor& visitor) const = 0;
	};

	class ErrorPacket : public Packet {
		private:
			uint8_t _code;

		public:
			ErrorPacket(uint8_t code, uint8_t flags = 0)
				: Packet(HXB_PTYPE_ERROR, flags), _code(code)
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
			EIDPacket(uint8_t type, uint32_t eid, uint8_t flags = 0)
				: Packet(type, flags), _eid(eid)
			{}

			uint32_t eid() const { return _eid; }
	};

	class QueryPacket : public EIDPacket {
		public:
			QueryPacket(uint32_t eid, uint8_t flags = 0)
				: EIDPacket(HXB_PTYPE_QUERY, eid, flags)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	class EndpointQueryPacket : public EIDPacket {
		public:
			EndpointQueryPacket(uint32_t eid, uint8_t flags = 0)
				: EIDPacket(HXB_PTYPE_EPQUERY, eid, flags)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	template<typename TValue>
	class ValuePacket : public EIDPacket {
		private:
			BOOST_STATIC_ASSERT_MSG((
				boost::is_same<TValue, bool>::value
					|| boost::is_same<TValue, uint8_t>::value
					|| boost::is_same<TValue, uint32_t>::value
					|| boost::is_same<TValue, float>::value
					|| boost::is_same<TValue, boost::posix_time::ptime>::value
					|| boost::is_same<TValue, boost::posix_time::time_duration>::value),
				"I don't know how to handle that type");

			TValue _value;

		protected:
			ValuePacket(uint8_t type, uint32_t eid, const TValue& value, uint8_t flags = 0)
				: EIDPacket(type, eid, flags), _value(value)
			{}

		public:
			const TValue& value() const { return _value; }
	};

	template<>
	class ValuePacket<std::string> : public EIDPacket {
		private:
			std::string _value;

		protected:
			ValuePacket(uint8_t type, uint32_t eid, const std::string& value, uint8_t flags = 0)
				: EIDPacket(type, eid, flags), _value(value)
			{
				if (value.size() > HXB_STRING_PACKET_MAX_BUFFER_LENGTH)
					throw std::out_of_range("value");
			}

		public:
			const std::string& value() const { return _value; }
	};

	template<>
	class ValuePacket<std::vector<char> > : public EIDPacket {
		private:
			std::vector<char> _value;

		protected:
			ValuePacket(uint8_t type, uint32_t eid, const std::vector<char>& value, uint8_t flags = 0)
				: EIDPacket(type, eid, flags), _value(value)
			{
				if (value.size() > HXB_BYTES_PACKET_MAX_BUFFER_LENGTH)
					throw std::out_of_range("value");
			}

		public:
			const std::vector<char>& value() const { return _value; }
	};

	template<typename TValue>
	class InfoPacket : public ValuePacket<TValue> {
		public:
			InfoPacket(uint32_t eid, const TValue& value, uint8_t flags = 0)
				: ValuePacket<TValue>(HXB_PTYPE_INFO, eid, value, flags)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	template<typename TValue>
	class WritePacket : public ValuePacket<TValue> {
		public:
			WritePacket(uint32_t eid, const TValue& value, uint8_t flags = 0)
				: ValuePacket<TValue>(HXB_PTYPE_WRITE, eid, value, flags)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};

	class EndpointInfoPacket : public ValuePacket<std::string> {
		public:
			EndpointInfoPacket(uint32_t eid, const std::string& value, uint8_t flags = 0)
				: ValuePacket(HXB_PTYPE_EPINFO, eid, value, flags)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};
};

#endif
