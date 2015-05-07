#ifndef LIBHEXABUS_PACKET_HPP
#define LIBHEXABUS_PACKET_HPP 1

#include <iostream>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>
#include <array>

#include <boost/asio/ip/address_v6.hpp>

#include <libhexabus/hexabus_types.h>

namespace hexabus {
	class PacketVisitor;

	class Packet {
		public:
			typedef std::shared_ptr<Packet> Ptr;

			enum Flags {
				want_ack = HXB_FLAG_WANT_ACK,
				want_ul_ack = HXB_FLAG_WANT_UL_ACK,
				reliable = HXB_FLAG_RELIABLE,
				conn_reset = HXB_FLAG_CONN_RESET
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
	class PropertyQueryPacket;
	template<typename TValue>
	class PropertyWritePacket;
	template<typename TValue>
	class PropertyReportPacket;

	class PacketVisitor {
		public:
			virtual ~PacketVisitor() {}

			virtual void visit(const ErrorPacket& error) = 0;
			virtual void visit(const QueryPacket& query) = 0;
			virtual void visit(const EndpointQueryPacket& endpointQuery) = 0;
			virtual void visit(const EndpointInfoPacket& endpointInfo) = 0;
			virtual void visit(const EndpointReportPacket& endpointReport) = 0;
			virtual void visit(const AckPacket& ack) = 0;
			virtual void visit(const PropertyQueryPacket& propertyQuery) = 0;

			virtual void visit(const InfoPacket<bool>& info) = 0;
			virtual void visit(const InfoPacket<uint8_t>& info) = 0;
			virtual void visit(const InfoPacket<uint16_t>& info) = 0;
			virtual void visit(const InfoPacket<uint32_t>& info) = 0;
			virtual void visit(const InfoPacket<uint64_t>& info) = 0;
			virtual void visit(const InfoPacket<int8_t>& info) = 0;
			virtual void visit(const InfoPacket<int16_t>& info) = 0;
			virtual void visit(const InfoPacket<int32_t>& info) = 0;
			virtual void visit(const InfoPacket<int64_t>& info) = 0;
			virtual void visit(const InfoPacket<float>& info) = 0;
			virtual void visit(const InfoPacket<std::string>& info) = 0;
			virtual void visit(const InfoPacket<std::array<uint8_t, 16> >& info) = 0;
			virtual void visit(const InfoPacket<std::array<uint8_t, 65> >& info) = 0;

			virtual void visit(const WritePacket<bool>& write) = 0;
			virtual void visit(const WritePacket<uint8_t>& write) = 0;
			virtual void visit(const WritePacket<uint16_t>& write) = 0;
			virtual void visit(const WritePacket<uint32_t>& write) = 0;
			virtual void visit(const WritePacket<uint64_t>& write) = 0;
			virtual void visit(const WritePacket<int8_t>& write) = 0;
			virtual void visit(const WritePacket<int16_t>& write) = 0;
			virtual void visit(const WritePacket<int32_t>& write) = 0;
			virtual void visit(const WritePacket<int64_t>& write) = 0;
			virtual void visit(const WritePacket<float>& write) = 0;
			virtual void visit(const WritePacket<std::string>& write) = 0;
			virtual void visit(const WritePacket<std::array<uint8_t, 16> >& write) = 0;
			virtual void visit(const WritePacket<std::array<uint8_t, 65> >& write) = 0;

			virtual void visit(const ReportPacket<bool>& report) = 0;
			virtual void visit(const ReportPacket<uint8_t>& report) = 0;
			virtual void visit(const ReportPacket<uint16_t>& report) = 0;
			virtual void visit(const ReportPacket<uint32_t>& report) = 0;
			virtual void visit(const ReportPacket<uint64_t>& report) = 0;
			virtual void visit(const ReportPacket<int8_t>& report) = 0;
			virtual void visit(const ReportPacket<int16_t>& report) = 0;
			virtual void visit(const ReportPacket<int32_t>& report) = 0;
			virtual void visit(const ReportPacket<int64_t>& report) = 0;
			virtual void visit(const ReportPacket<float>& report) = 0;
			virtual void visit(const ReportPacket<std::string>& report) = 0;
			virtual void visit(const ReportPacket<std::array<uint8_t, 16> >& report) = 0;
			virtual void visit(const ReportPacket<std::array<uint8_t, 65> >& report) = 0;

			virtual void visit(const ProxyInfoPacket<bool>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<uint8_t>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<uint16_t>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<uint32_t>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<uint64_t>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<int8_t>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<int16_t>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<int32_t>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<int64_t>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<float>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<std::string>& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<std::array<uint8_t, 16> >& proxyInfo) = 0;
			virtual void visit(const ProxyInfoPacket<std::array<uint8_t, 65> >& proxyInfo) = 0;

			virtual void visit(const PropertyWritePacket<bool>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<uint8_t>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<uint16_t>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<uint32_t>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<uint64_t>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<int8_t>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<int16_t>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<int32_t>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<int64_t>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<float>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<std::string>& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<std::array<uint8_t, 16> >& propertyWrite) = 0;
			virtual void visit(const PropertyWritePacket<std::array<uint8_t, 65> >& propertyWrite) = 0;

			virtual void visit(const PropertyReportPacket<bool>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<uint8_t>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<uint16_t>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<uint32_t>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<uint64_t>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<int8_t>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<int16_t>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<int32_t>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<int64_t>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<float>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<std::string>& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<std::array<uint8_t, 16> >& propertyReport) = 0;
			virtual void visit(const PropertyReportPacket<std::array<uint8_t, 65> >& propertyReport) = 0;

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
			static_assert(
				std::is_same<TValue, bool>::value
					|| std::is_same<TValue, uint8_t>::value
					|| std::is_same<TValue, uint16_t>::value
					|| std::is_same<TValue, uint32_t>::value
					|| std::is_same<TValue, uint64_t>::value
					|| std::is_same<TValue, int8_t>::value
					|| std::is_same<TValue, int16_t>::value
					|| std::is_same<TValue, int32_t>::value
					|| std::is_same<TValue, int64_t>::value
					|| std::is_same<TValue, float>::value
					|| std::is_same<TValue, std::array<uint8_t, 16>>::value
					|| std::is_same<TValue, std::array<uint8_t, 65>>::value,
				"I don't know how to handle that type");

			static uint8_t calculateDatatype()
			{
				return
					std::is_same<TValue, bool>::value ? HXB_DTYPE_BOOL :
					std::is_same<TValue, uint8_t>::value ? HXB_DTYPE_UINT8 :
					std::is_same<TValue, uint16_t>::value ? HXB_DTYPE_UINT16 :
					std::is_same<TValue, uint32_t>::value ? HXB_DTYPE_UINT32 :
					std::is_same<TValue, uint64_t>::value ? HXB_DTYPE_UINT64 :
					std::is_same<TValue, int8_t>::value ? HXB_DTYPE_SINT8 :
					std::is_same<TValue, int16_t>::value ? HXB_DTYPE_SINT16 :
					std::is_same<TValue, int32_t>::value ? HXB_DTYPE_SINT32 :
					std::is_same<TValue, int64_t>::value ? HXB_DTYPE_SINT64 :
					std::is_same<TValue, float>::value ? HXB_DTYPE_FLOAT :
					std::is_same<TValue, std::array<uint8_t, 16>>::value ? HXB_DTYPE_16BYTES : HXB_DTYPE_65BYTES;
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
	};

	template<typename TValue>
	class PropertyReportPacket : public ValuePacket<TValue>, public CausedPacket {
		private:
			uint32_t _nextid;
		public:
			PropertyReportPacket(uint32_t nextid, int16_t cause, uint32_t eid, const TValue& value, uint8_t flags = 0, uint16_t sequenceNumber = 0)
				: ValuePacket<TValue>(HXB_PTYPE_EP_PROP_REPORT, eid, value, flags, sequenceNumber), CausedPacket(cause), _nextid(nextid)
			{}

			uint32_t nextid() const { return _nextid; }

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};



	namespace detail {
		template<bool FailOnIncomplete, typename... Actions>
		class DispatchVisitor : public PacketVisitor {
		private:
			std::tuple<Actions*...> _actions;

			template<typename Action, typename P, typename = decltype(std::declval<Action>()(std::declval<P>()))>
			static constexpr bool matchesDirect(int) { return true; }
			template<typename Action, typename P>
			static constexpr bool matchesDirect(void*) { return false; }

			template<typename Action, template<typename> class PT>
			static constexpr bool matchesIndirect()
			{
				return matchesDirect<Action, PT<bool>>(0) ||
					matchesDirect<Action, PT<uint8_t>>(0) ||
					matchesDirect<Action, PT<uint16_t>>(0) ||
					matchesDirect<Action, PT<uint32_t>>(0) ||
					matchesDirect<Action, PT<uint64_t>>(0) ||
					matchesDirect<Action, PT<int8_t>>(0) ||
					matchesDirect<Action, PT<int16_t>>(0) ||
					matchesDirect<Action, PT<int32_t>>(0) ||
					matchesDirect<Action, PT<int64_t>>(0) ||
					matchesDirect<Action, PT<float>>(0) ||
					matchesDirect<Action, PT<std::string>>(0) ||
					matchesDirect<Action, PT<std::array<uint8_t, 16>>>(0) ||
					matchesDirect<Action, PT<std::array<uint8_t, 65>>>(0);
			}

			template<typename P>
			void invoke(const P&, std::integral_constant<unsigned, sizeof...(Actions)>, int)
			{
			}

			template<typename P, unsigned Idx>
			auto invoke(const P& p, std::integral_constant<unsigned, Idx> idx, int)
				-> typename std::conditional<
					true,
					void,
					decltype(std::declval<typename std::tuple_element<Idx, std::tuple<Actions..., void>>::type>()(p))>::type
			{
				(*std::get<Idx>(_actions))(p);
			}

			template<typename P, unsigned Idx>
			void invoke(const P& p, std::integral_constant<unsigned, Idx> idx, void*)
			{
				typedef typename std::tuple_element<Idx, std::tuple<Actions...>>::type action_t;
				static_assert(
					matchesDirect<action_t, ErrorPacket>(0) ||
					matchesDirect<action_t, QueryPacket>(0) ||
					matchesDirect<action_t, EndpointQueryPacket>(0) ||
					matchesDirect<action_t, EndpointInfoPacket>(0) ||
					matchesDirect<action_t, EndpointReportPacket>(0) ||
					matchesDirect<action_t, AckPacket>(0) ||
					matchesDirect<action_t, PropertyQueryPacket>(0) ||
					matchesIndirect<action_t, InfoPacket>() ||
					matchesIndirect<action_t, WritePacket>() ||
					matchesIndirect<action_t, ProxyInfoPacket>() ||
					matchesIndirect<action_t, PropertyWritePacket>() ||
					matchesIndirect<action_t, PropertyReportPacket>(),
					"Action not callable for any packet type");

				static_assert(!(FailOnIncomplete && Idx + 1 == sizeof...(Actions)), "Incomplete match");
				invoke(p, std::integral_constant<unsigned, Idx + 1>(), 0);
			}

			template<typename P>
			void invoke(const P& p)
			{
				invoke(p, std::integral_constant<unsigned, 0>(), 0);
			}

		public:
			DispatchVisitor(Actions&... actions)
				: _actions(&actions...)
			{}

			virtual void visit(const ErrorPacket& packet) { invoke(packet); }
			virtual void visit(const QueryPacket& packet) { invoke(packet); }
			virtual void visit(const EndpointQueryPacket& packet) { invoke(packet); }
			virtual void visit(const EndpointInfoPacket& packet) { invoke(packet); }
			virtual void visit(const EndpointReportPacket& packet) { invoke(packet); }
			virtual void visit(const AckPacket& packet) { invoke(packet); }
			virtual void visit(const PropertyQueryPacket& packet) { invoke(packet); }

			virtual void visit(const InfoPacket<bool>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<uint8_t>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<uint16_t>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<uint32_t>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<uint64_t>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<int8_t>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<int16_t>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<int32_t>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<int64_t>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<float>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<std::string>& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<std::array<uint8_t, 16> >& packet) { invoke(packet); }
			virtual void visit(const InfoPacket<std::array<uint8_t, 65> >& packet) { invoke(packet); }

			virtual void visit(const WritePacket<bool>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<uint8_t>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<uint16_t>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<uint32_t>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<uint64_t>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<int8_t>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<int16_t>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<int32_t>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<int64_t>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<float>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<std::string>& packet) { invoke(packet); }
			virtual void visit(const WritePacket<std::array<uint8_t, 16> >& packet) { invoke(packet); }
			virtual void visit(const WritePacket<std::array<uint8_t, 65> >& packet) { invoke(packet); }

			virtual void visit(const ReportPacket<bool>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<uint8_t>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<uint16_t>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<uint32_t>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<uint64_t>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<int8_t>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<int16_t>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<int32_t>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<int64_t>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<float>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<std::string>& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<std::array<uint8_t, 16> >& packet) { invoke(packet); }
			virtual void visit(const ReportPacket<std::array<uint8_t, 65> >& packet) { invoke(packet); }

			virtual void visit(const ProxyInfoPacket<bool>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<uint8_t>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<uint16_t>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<uint32_t>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<uint64_t>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<int8_t>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<int16_t>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<int32_t>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<int64_t>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<float>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<std::string>& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<std::array<uint8_t, 16> >& packet) { invoke(packet); }
			virtual void visit(const ProxyInfoPacket<std::array<uint8_t, 65> >& packet) { invoke(packet); }

			virtual void visit(const PropertyWritePacket<bool>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<uint8_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<uint16_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<uint32_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<uint64_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<int8_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<int16_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<int32_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<int64_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<float>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<std::string>& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<std::array<uint8_t, 16> >& packet) { invoke(packet); }
			virtual void visit(const PropertyWritePacket<std::array<uint8_t, 65> >& packet) { invoke(packet); }

			virtual void visit(const PropertyReportPacket<bool>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<uint8_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<uint16_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<uint32_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<uint64_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<int8_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<int16_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<int32_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<int64_t>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<float>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<std::string>& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<std::array<uint8_t, 16> >& packet) { invoke(packet); }
			virtual void visit(const PropertyReportPacket<std::array<uint8_t, 65> >& packet) { invoke(packet); }

			virtual void visitPacket(const Packet& packet)
			{
				packet.accept(*this);
			}
		};
	}

	template<typename... Actions>
	inline void dispatch(const Packet& packet, Actions&&... actions)
	{
		detail::DispatchVisitor<false, typename std::decay<Actions>::type...> visitor(actions...);
		packet.accept(visitor);
	}

	template<typename... Actions>
	inline void dispatchAll(const Packet& packet, Actions&&... actions)
	{
		detail::DispatchVisitor<true, typename std::decay<Actions>::type...> visitor(actions...);
		packet.accept(visitor);
	}

	namespace detail {
		struct CloneAction {
			Packet::Ptr result;

			template<typename P>
			void operator()(const P& packet)
			{
				result.reset(new auto(packet));
			}
		};
	}

	inline Packet::Ptr clone(const Packet& packet)
	{
		detail::CloneAction ca;
		dispatchAll(packet, ca);
		return ca.result;
	}

};

#endif
