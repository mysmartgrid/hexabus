#ifndef LIBHEXABUS_PACKET_HPP
#define LIBHEXABUS_PACKET_HPP 1

#include <iostream>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>
#include <array>

#include <libhexabus/hexabus_types.h>

namespace hexabus {
	class PacketVisitor;

	class Packet {
		public:
			typedef std::shared_ptr<Packet> Ptr;

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

			virtual void visitPacket(const Packet& packet)
			{
				packet.accept(*this);
			}
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

	class TypedPacket : public EIDPacket {
		private:
			uint8_t _datatype;

		public:
			TypedPacket(uint8_t type, uint32_t eid, uint8_t datatype, uint8_t flags = 0)
				: EIDPacket(type, eid, flags), _datatype(datatype)
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
			ValuePacket(uint8_t type, uint32_t eid, const TValue& value, uint8_t flags = 0)
				: TypedPacket(type, eid, calculateDatatype(), flags), _value(value)
			{}

		public:
			const TValue& value() const { return _value; }
	};

	template<>
	class ValuePacket<std::string> : public TypedPacket {
		private:
			std::string _value;

		protected:
			ValuePacket(uint8_t type, uint32_t eid, const std::string& value, uint8_t flags = 0)
				: TypedPacket(type, eid, HXB_DTYPE_128STRING, flags), _value(value)
			{
				if (value.size() > max_length)
					throw std::out_of_range("value");
			}

			ValuePacket(uint8_t type, uint32_t eid, uint8_t datatype, const std::string& value, uint8_t flags = 0)
				: TypedPacket(type, eid, datatype, flags), _value(value)
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
			EndpointInfoPacket(uint32_t eid, uint8_t datatype, const std::string& value, uint8_t flags = 0)
				: ValuePacket<std::string>(HXB_PTYPE_EPINFO, eid, datatype, value, flags)
			{}

			virtual void accept(PacketVisitor& visitor) const
			{
				visitor.visit(*this);
			}
	};
};

#endif
