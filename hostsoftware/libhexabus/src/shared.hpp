#ifndef SRC_SHARED_HPP_04D73A47C8DF11F7
#define SRC_SHARED_HPP_04D73A47C8DF11F7

#include <string>

#include <boost/asio.hpp>
#include <boost/format.hpp>

#include "../../../shared/hexabus_definitions.h"

namespace hexabus {

inline std::string errcodeStr(uint8_t code)
{
	switch ((hxb_error_code) code) {
	case HXB_ERR_SUCCESS: return "Success";
	case HXB_ERR_UNKNOWNEID: return "Unknown EID";
	case HXB_ERR_WRITEREADONLY: return "Write on readonly endpoint";
	case HXB_ERR_CRCFAILED: return "CRC failed";
	case HXB_ERR_DATATYPE: return "Datatype mismatch";
	case HXB_ERR_INVALID_VALUE: return "Invalid value";

	case HXB_ERR_MALFORMED_PACKET: return "(malformaed packet)";
	case HXB_ERR_UNEXPECTED_PACKET: return "(unexpected packet)";
	case HXB_ERR_NO_VALUE: return "(no value)";
	case HXB_ERR_INVALID_WRITE: return "(invalid write)";
	}

	return str(boost::format("(error %1%)") % unsigned(code));
}

inline std::string shortDatatypeName(hxb_datatype type)
{
	switch (type) {
	case HXB_DTYPE_BOOL: return "b";
	case HXB_DTYPE_UINT8: return "u8";
	case HXB_DTYPE_UINT16: return "u16";
	case HXB_DTYPE_UINT32: return "u32";
	case HXB_DTYPE_UINT64: return "u64";
	case HXB_DTYPE_SINT8: return "s8";
	case HXB_DTYPE_SINT16: return "s16";
	case HXB_DTYPE_SINT32: return "s32";
	case HXB_DTYPE_SINT64: return "s64";
	case HXB_DTYPE_FLOAT: return "f";
	case HXB_DTYPE_128STRING: return "s";
	case HXB_DTYPE_65BYTES: return "b65";
	case HXB_DTYPE_16BYTES: return "b16";
	default: return "(unknown)";
	}
}

inline int dtypeStrToDType(const std::string& s)
{
	for (hxb_datatype type = HXB_DTYPE_BOOL; type <= HXB_DTYPE_16BYTES; type = hxb_datatype(type + 1))
		if (shortDatatypeName(type) == s || datatypeName(type) == s)
			return type;

	return -1;
}

class PacketPrinter : private hexabus::PacketVisitor {
private:
	enum FieldName {
		F_FROM,
		F_VALUE,
		F_EID,
		F_DATATYPE,
		F_TYPE,
		F_ERROR_CODE,
		F_ERROR_STR,
	};

private:
	bool oneline;

	std::stringstream buffer;

	void beginField(FieldName fieldName)
	{
		if (buffer.tellp())
			buffer << (oneline ? ';' : '\n');

		std::string fieldNameStr;
		switch (fieldName) {
		case F_FROM:       fieldNameStr = "Packet from"; break;
		case F_VALUE:      fieldNameStr = "Value"; break;
		case F_EID:        fieldNameStr = "EID"; break;
		case F_DATATYPE:   fieldNameStr = "Datatype"; break;
		case F_TYPE:       fieldNameStr = "Type"; break;
		case F_ERROR_CODE: fieldNameStr = "Error code"; break;
		case F_ERROR_STR:  fieldNameStr = "Error"; break;
		}

		buffer << fieldNameStr << std::string(oneline ? 1 : 14 - fieldNameStr.size(), ' ');
	}

	template<typename Value>
	void addField(FieldName name, const Value& value)
	{
		beginField(name);
		buffer << value;
	}

	void addField(FieldName name, const std::string& value)
	{
		beginField(name);

		for (auto c : value) {
			if ((oneline && (std::isblank(c) || std::isprint(c)))
					|| (!oneline && std::isprint(c)))
				buffer << c;
			else
				buffer << str(boost::format("\\x%02x") % (0xff & c));
		}
	}

	template<size_t L>
	void addField(FieldName name, const std::array<uint8_t, L>& value)
	{
		beginField(name);

		for (auto c : value)
			buffer << str(boost::format("%02x") % unsigned(c));
	}

	template<typename T>
	void printValuePacket(const hexabus::ValuePacket<T>& packet, const std::string& type)
	{
		addField(F_TYPE, type);
		addField(F_EID, packet.eid());
		addField(F_DATATYPE, hexabus::datatypeName((hexabus::hxb_datatype) packet.datatype()));

		typedef typename std::conditional<
			std::is_same<T, uint8_t>::value,
			unsigned,
			typename std::conditional<
				std::is_same<T, int8_t>::value,
				int,
				const T&>::type
			>::type Widened;
		addField(F_VALUE, Widened(packet.value()));
	}

	virtual void visit(const hexabus::ErrorPacket& error)
	{
		addField(F_TYPE, "Error");
		addField(F_ERROR_CODE, unsigned(error.code()));
		addField(F_ERROR_STR, errcodeStr(error.code()));
	}

	virtual void visit(const hexabus::QueryPacket& query)
	{
		addField(F_TYPE, "Query");
		addField(F_EID, query.eid());
	}

	virtual void visit(const hexabus::EndpointQueryPacket& query)
	{
		addField(F_TYPE, "EPQuery");
		addField(F_EID, query.eid());
	}

	virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo)
	{
		addField(F_TYPE, "EPInfo");
		addField(F_EID, endpointInfo.eid());
		addField(F_DATATYPE, hexabus::datatypeName((hexabus::hxb_datatype) endpointInfo.datatype()));
		addField(F_VALUE, endpointInfo.value());
	}

	virtual void visit(const hexabus::InfoPacket<bool>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<uint8_t>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<uint16_t>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<uint32_t>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<uint64_t>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<int8_t>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<int16_t>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<int32_t>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<int64_t>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<float>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<std::string>& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<std::array<uint8_t, 16> >& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<std::array<uint8_t, 65> >& info) { printValuePacket(info, "Info"); }

	virtual void visit(const hexabus::WritePacket<bool>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<uint8_t>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<uint16_t>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<uint32_t>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<uint64_t>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<int8_t>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<int16_t>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<int32_t>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<int64_t>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<float>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<std::string>& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<std::array<uint8_t, 16> >& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<std::array<uint8_t, 65> >& write) { printValuePacket(write, "Write"); }

public:
	PacketPrinter(bool oneline)
		: oneline(oneline)
	{}

	void operator()(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
	{
		addField(F_FROM, from);
		p.accept(*this);
		std::cout << buffer.str() << std::endl;
		if (!oneline)
			std::cout << std::endl;
		buffer.str("");
	}
};



boost::asio::ip::address_v6 resolve(boost::asio::io_service& io, const std::string& host, boost::system::error_code& err)
{
	boost::asio::ip::udp::resolver resolver(io);

	boost::asio::ip::udp::resolver::query query(host, "");

	boost::asio::ip::udp::resolver::iterator it, end;

	it = resolver.resolve(query, err);
	if (err)
		return boost::asio::ip::address_v6::any();

	if (it == end || !it->endpoint().address().is_v6()) {
		err = boost::system::error_code(boost::system::errc::invalid_argument, boost::system::generic_category());
		return boost::asio::ip::address_v6::any();
	}

	return it->endpoint().address().to_v6();
}

}

#endif
