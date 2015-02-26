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
	case HXB_ERR_DATATYPE: return "Datatype mismatch";
	case HXB_ERR_INVALID_VALUE: return "Invalid value";

	case HXB_ERR_MALFORMED_PACKET: return "(malformaed packet)";
	case HXB_ERR_UNEXPECTED_PACKET: return "(unexpected packet)";
	case HXB_ERR_NO_VALUE: return "(no value)";
	case HXB_ERR_INVALID_WRITE: return "(invalid write)";
	case HXB_ERR_OUT_OF_MEMORY: return "(out of memory)";
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
		F_SEQNUM,
		F_EID,
		F_DATATYPE,
		F_TYPE,
		F_PROPID,
		F_NEXTPROP,
		F_CAUSE,
		F_ORIGIN,
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
		case F_SEQNUM:     fieldNameStr = "Sequence Number"; break;
		case F_EID:        fieldNameStr = "EID"; break;
		case F_DATATYPE:   fieldNameStr = "Datatype"; break;
		case F_TYPE:       fieldNameStr = "Type"; break;
		case F_PROPID:     fieldNameStr = "PropID"; break;
		case F_NEXTPROP:   fieldNameStr = "Next PropID"; break;
		case F_CAUSE:      fieldNameStr = "Cause"; break;
		case F_ORIGIN:     fieldNameStr = "Origin IP"; break;
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

	template<typename T>
	void printReportPacket(const hexabus::ReportPacket<T>& packet)
	{
		printValuePacket(packet, "Report");
		addField(F_CAUSE, packet.cause());
	}

	template<typename T>
	void printProxyInfoPacket(const hexabus::ProxyInfoPacket<T>& packet)
	{
		printValuePacket(packet, "PInfo");
		addField(F_ORIGIN, packet.origin().to_string());
	}

	template<typename T>
	void printPropertyWritePacket(const hexabus::PropertyWritePacket<T>& packet)
	{
		addField(F_PROPID, packet.propid());
		printValuePacket(packet, "PropWrite");
	}

	template<typename T>
	void printPropertyReportPacket(const hexabus::PropertyReportPacket<T>& packet)
	{
		printValuePacket(packet, "PropReport");
		addField(F_NEXTPROP, packet.nextid());
		addField(F_CAUSE, packet.cause());
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

	virtual void visit(const hexabus::EndpointReportPacket& endpointReport)
	{
		addField(F_TYPE, "EPReport");
		addField(F_EID, endpointReport.eid());
		addField(F_DATATYPE, hexabus::datatypeName((hexabus::hxb_datatype) endpointReport.datatype()));
		addField(F_VALUE, endpointReport.value());
		addField(F_CAUSE, endpointReport.cause());
	}

	virtual void visit(const hexabus::AckPacket& ack)
	{
		addField(F_TYPE, "Ack");
		addField(F_CAUSE, ack.cause());
	}

	virtual void visit(const hexabus::PropertyQueryPacket& propertyQuery)
	{
		addField(F_TYPE, "PropQuery");
		addField(F_EID, propertyQuery.eid());
		addField(F_PROPID, propertyQuery.propid());
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

	virtual void visit(const hexabus::ReportPacket<bool>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<uint8_t>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<uint16_t>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<uint32_t>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<uint64_t>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<int8_t>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<int16_t>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<int32_t>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<int64_t>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<float>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<std::string>& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<std::array<uint8_t, 16> >& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<std::array<uint8_t, 65> >& report) { printReportPacket(report); }

	virtual void visit(const hexabus::ProxyInfoPacket<bool>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<uint8_t>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<uint16_t>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<uint32_t>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<uint64_t>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<int8_t>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<int16_t>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<int32_t>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<int64_t>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<float>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<std::string>& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<std::array<uint8_t, 16> >& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<std::array<uint8_t, 65> >& pinfo) { printProxyInfoPacket(pinfo); }

	virtual void visit(const hexabus::PropertyWritePacket<bool>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<uint8_t>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<uint16_t>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<uint32_t>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<uint64_t>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<int8_t>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<int16_t>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<int32_t>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<int64_t>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<float>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<std::string>& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<std::array<uint8_t, 16> >& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<std::array<uint8_t, 65> >& propwrite) { printPropertyWritePacket(propwrite); }

	virtual void visit(const hexabus::PropertyReportPacket<bool>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<uint8_t>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<uint16_t>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<uint32_t>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<uint64_t>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<int8_t>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<int16_t>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<int32_t>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<int64_t>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<float>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<std::string>& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<std::array<uint8_t, 16> >& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<std::array<uint8_t, 65> >& propreport) { printPropertyReportPacket(propreport); }

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
