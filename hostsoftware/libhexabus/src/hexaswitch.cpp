#include <iostream>
#include <string.h>
#include <iomanip>
#include <libhexabus/common.hpp>
#include <libhexabus/liveness.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <time.h>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <typeinfo>
namespace po = boost::program_options;

#include "../../../shared/endpoints.h"
#include "../../../shared/hexabus_definitions.h"
#include "resolv.hpp"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

using namespace hexabus;

enum ErrorCode {
	ERR_NONE = 0,

	ERR_UNKNOWN_PARAMETER = 1,
	ERR_PARAMETER_MISSING = 2,
	ERR_PARAMETER_FORMAT = 3,
	ERR_PARAMETER_VALUE_INVALID = 4,
	ERR_NETWORK = 5,

	ERR_OTHER = 127
};

static std::string errcodeStr(uint8_t code)
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
	return "";
}

static bool oneline, json;

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

	struct Field {
		FieldName name;
		std::string value;
		bool unformatted;
	};

	std::ostream& target;
	std::vector<Field> fields;

	void flush()
	{
		if (json)
			target << '{';

		unsigned i = 0;
		for (const auto& field : fields) {
			if (i++) {
				if (json)
					target << ", ";
				else if (oneline)
					target << ";";
				else
					target << std::endl;
			}

			std::string name;
			switch (field.name) {
			case F_FROM: name = (json ? R"("from")" : "Packet from"); break;
			case F_VALUE: name = (json ? R"("value")" : "Value"); break;
			case F_SEQNUM: name = (json ? R"("sequenceNumber")" : "Sequence Number"); break;
			case F_EID: name = (json ? R"("eid")" : "EID"); break;
			case F_DATATYPE: name = (json ? R"("datatype")" : "Datatype"); break;
			case F_TYPE: name = (json ? R"("type")" : "Type"); break;
			case F_PROPID: name = (json ? R"("propid")" : "Property ID"); break;
			case F_NEXTPROP: name = (json ? R"("nextid")" : "Next property ID"); break;
			case F_CAUSE: name = (json ? R"("cause")" : "Cause"); break;
			case F_ORIGIN: name = (json ? R"("origin")" : "Origin IP"); break;
			case F_ERROR_CODE: name = (json ? R"("error_code")" : "Error code"); break;
			case F_ERROR_STR: name = (json ? R"("error_str")" : "Error"); break;
			}

			target << name;

			if (json) {
				target << ": ";
			} else if (oneline) {
				target << ' ';
			} else {
				target << ": ";
				// pad to length of longest field designator and a bit
				for (int l = 12 - name.size(); l >= 0; --l)
					target << ' ';
			}

			if (field.unformatted && json)
				target << format(field.value);
			else
				target << field.value;
		}
		fields.clear();

		if (json)
			target << '}';
		else if (!oneline)
			target << std::endl;

		target << std::endl;
	}

	static std::string format(const std::string& value)
	{
		std::stringstream sbuf;

		sbuf << '"';
		for (auto c : value) {
			switch (c) {
			case '"': sbuf << "\\\""; break;
			case '\\': sbuf << "\\\\"; break;
			case '\n': sbuf << "\\n"; break;
			case '\r': sbuf << "\\r"; break;
			case '\t': sbuf << "\\t"; break;
			case '\b': sbuf << "\\b"; break;
			case '\f': sbuf << "\\f"; break;
			default: sbuf << c;
			}
		}
		sbuf << '"';
		return sbuf.str();
	}

	template<size_t L>
	static std::string format(const boost::array<char, L>& value)
	{
		std::stringstream hexstream;
		const char* sep = json ? ", 0x" : " ";

		if (json)
			hexstream << "[0x";

		hexstream << std::hex << std::setfill('0');

		for (size_t i = 0; i < L; ++i)
			hexstream << (i ? sep : "") << std::setw(2) << (0xFF & (unsigned char)(value[i]));

		if (json)
			hexstream << ']';

		return hexstream.str();
	}

	template<typename T>
	static std::string format(T value)
	{
		static_assert(std::is_arithmetic<T>::value, "");

		std::stringstream sbuf;
		sbuf << value;
		return sbuf.str();
	}

	template<typename T>
	void printValuePacket(const hexabus::ValuePacket<T>& packet, const char* type)
	{
		fields.push_back({ F_TYPE, type, true });
		fields.push_back({ F_EID, format(packet.eid()), false });
		fields.push_back({ F_DATATYPE, hexabus::datatypeName((hexabus::hxb_datatype) packet.datatype()), true });
		fields.push_back({ F_VALUE, format(packet.value()), false });
	}

	template<typename T>
	void printReportPacket(const hexabus::ReportPacket<T>& packet)
	{
		printValuePacket(packet, "Report");
		fields.push_back({ F_CAUSE, format(packet.cause()), true });
	}

	template<typename T>
	void printProxyInfoPacket(const hexabus::ProxyInfoPacket<T>& packet)
	{
		printValuePacket(packet, "PInfo");
		fields.push_back({ F_ORIGIN, packet.origin().to_string(), true });
	}

	template<typename T>
	void printPropertyWritePacket(const hexabus::PropertyWritePacket<T>& packet)
	{
		fields.push_back({ F_PROPID, format(packet.propid()), true });
		printValuePacket(packet, "PropWrite");
	}

	template<typename T>
	void printPropertyReportPacket(const hexabus::PropertyReportPacket<T>& packet)
	{
		printValuePacket(packet, "PropReport");
		fields.push_back({ F_NEXTPROP, format(packet.nextid()), true });
		fields.push_back({ F_CAUSE, format(packet.cause()), true });
	}

	virtual void visit(const hexabus::ErrorPacket& error)
	{
		fields.push_back({ F_TYPE, "Error", true });
		fields.push_back({ F_ERROR_CODE, format(error.code()), false });
		fields.push_back({ F_ERROR_STR, errcodeStr(error.code()), true });
		fields.push_back({ F_CAUSE, format(error.cause()), true });
	}

	virtual void visit(const hexabus::QueryPacket& query)
	{
		fields.push_back({ F_TYPE, "Query", true });
		fields.push_back({ F_EID, format(query.eid()), false });
	}

	virtual void visit(const hexabus::EndpointQueryPacket& query)
	{
		fields.push_back({ F_TYPE, "EPQuery", true });
		fields.push_back({ F_EID, format(query.eid()), false });
	}

	virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo)
	{
		fields.push_back({ F_TYPE, "EPInfo", true });
		fields.push_back({ F_EID, format(endpointInfo.eid()), false });
		fields.push_back({ F_DATATYPE, hexabus::datatypeName((hexabus::hxb_datatype) endpointInfo.datatype()), true });
		fields.push_back({ F_VALUE, format(endpointInfo.value()), false });
	}

	virtual void visit(const hexabus::EndpointReportPacket& endpointReport)
	{
		fields.push_back({ F_TYPE, "EPReport", true });
		fields.push_back({ F_EID, format(endpointReport.eid()), false });
		fields.push_back({ F_DATATYPE, hexabus::datatypeName((hexabus::hxb_datatype) endpointReport.datatype()), true });
		fields.push_back({ F_VALUE, format(endpointReport.value()), false });
		fields.push_back({ F_CAUSE, format(endpointReport.cause()), true });
	}

	virtual void visit(const hexabus::AckPacket& ack)
	{
		fields.push_back({ F_TYPE, "Ack", true });
		fields.push_back({ F_CAUSE, format(ack.cause()), true });
	}

	virtual void visit(const hexabus::PropertyQueryPacket& propertyQuery)
	{
		fields.push_back({ F_TYPE, "PropQuery", true});
		fields.push_back({ F_EID, format(propertyQuery.eid()), false });
		fields.push_back({ F_PROPID, format(propertyQuery.propid()), false });
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
	virtual void visit(const hexabus::InfoPacket<boost::array<char, 16> >& info) { printValuePacket(info, "Info"); }
	virtual void visit(const hexabus::InfoPacket<boost::array<char, 65> >& info) { printValuePacket(info, "Info"); }

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
	virtual void visit(const hexabus::WritePacket<boost::array<char, 16> >& write) { printValuePacket(write, "Write"); }
	virtual void visit(const hexabus::WritePacket<boost::array<char, 65> >& write) { printValuePacket(write, "Write"); }

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
	virtual void visit(const hexabus::ReportPacket<boost::array<char, 16> >& report) { printReportPacket(report); }
	virtual void visit(const hexabus::ReportPacket<boost::array<char, 65> >& report) { printReportPacket(report); }

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
	virtual void visit(const hexabus::ProxyInfoPacket<boost::array<char, 16> >& pinfo) { printProxyInfoPacket(pinfo); }
	virtual void visit(const hexabus::ProxyInfoPacket<boost::array<char, 65> >& pinfo) { printProxyInfoPacket(pinfo); }

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
	virtual void visit(const hexabus::PropertyWritePacket<boost::array<char, 16> >& propwrite) { printPropertyWritePacket(propwrite); }
	virtual void visit(const hexabus::PropertyWritePacket<boost::array<char, 65> >& propwrite) { printPropertyWritePacket(propwrite); }

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
	virtual void visit(const hexabus::PropertyReportPacket<boost::array<char, 16> >& propreport) { printPropertyReportPacket(propreport); }
	virtual void visit(const hexabus::PropertyReportPacket<boost::array<char, 65> >& propreport) { printPropertyReportPacket(propreport); }

public:
	PacketPrinter(std::ostream& target)
		: target(target)
	{}

	void printPacket(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
	{
		fields.push_back({ F_FROM, from.address().to_string(), true });
		fields.push_back({ F_SEQNUM, format(p.sequenceNumber()), true });
		p.accept(*this);
		flush();
	}

	void printPacket(const hexabus::Packet& p)
	{
		p.accept(*this);
		flush();
	}
};

void print_packet(const hexabus::Packet& packet)
{
	PacketPrinter pp(std::cout);

	pp.printPacket(packet);
}

struct send_packet_trn_callback {
	boost::asio::io_service* io;
	ErrorCode err;

	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep, bool failed)
	{
		io->stop();
		if(failed) {
			std::cerr<<"Ack timeout!"<<std::endl;
			err = ERR_NETWORK;
		}
	}
};

ErrorCode send_packet(hexabus::Socket& net, const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet)
{
	send_packet_trn_callback tcb = {&net.ioService(), ERR_NONE};
	net.onPacketTransmitted(boost::ref(tcb), packet, boost::asio::ip::udp::endpoint(addr, 61616));

	net.ioService().reset();
	net.ioService().run();

	return tcb.err;
}

template<typename Filter>
ErrorCode send_packet_wait(hexabus::Socket& net, const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet, const Filter& filter)
{
	ErrorCode err;

	if ((err = send_packet(net, addr, packet))) {
		return err;
	}

	try {
		namespace hf = hexabus::filtering;
		std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> p = net.receive((filter || hf::isError()) && hf::sourceIP() == addr);

		print_packet(*p.first);
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Error receiving packet: " << e.code().message() << std::endl;
		return ERR_NETWORK;
	} catch (const hexabus::GenericException& e) {
		std::cerr << "Error receiving packet: " << e.what() << std::endl;
		return ERR_NETWORK;
	}

	return ERR_NONE;
}

template<template<typename TValue> class Packet, typename Content, typename ParsedType>
ErrorCode send_parsed_value(hexabus::Socket& sock, const boost::asio::ip::address_v6& ip, uint32_t eid,
		uint8_t flags, const std::string& valueStr)
{
	ParsedType value = boost::lexical_cast<ParsedType>(valueStr);

	std::cout << "Sending value " << value << std::endl;
	return send_packet(sock, ip, Packet<Content>(eid, value, flags));
}

template<template<typename TValue> class ValuePacket>
ErrorCode send_value_packet(hexabus::Socket& net, const boost::asio::ip::address_v6& ip, uint32_t eid, uint8_t dt,
		uint8_t flags, const std::string& value)
{
	try {
		switch ((hxb_datatype) dt) {
		case HXB_DTYPE_BOOL: return send_parsed_value<ValuePacket, bool, bool>(net, ip, eid, flags, value);
		case HXB_DTYPE_UINT8: return send_parsed_value<ValuePacket, uint8_t, unsigned>(net, ip, eid, flags, value);
		case HXB_DTYPE_UINT16: return send_parsed_value<ValuePacket, uint16_t, uint16_t>(net, ip, eid, flags, value);
		case HXB_DTYPE_UINT32: return send_parsed_value<ValuePacket, uint32_t, uint32_t>(net, ip, eid, flags, value);
		case HXB_DTYPE_UINT64: return send_parsed_value<ValuePacket, uint64_t, uint64_t>(net, ip, eid, flags, value);
		case HXB_DTYPE_SINT8: return send_parsed_value<ValuePacket, int8_t, int>(net, ip, eid, flags, value);
		case HXB_DTYPE_SINT16: return send_parsed_value<ValuePacket, int16_t, int16_t>(net, ip, eid, flags, value);
		case HXB_DTYPE_SINT32: return send_parsed_value<ValuePacket, int32_t, int32_t>(net, ip, eid, flags, value);
		case HXB_DTYPE_SINT64: return send_parsed_value<ValuePacket, int64_t, int32_t>(net, ip, eid, flags, value);
		case HXB_DTYPE_FLOAT: return send_parsed_value<ValuePacket, float, float>(net, ip, eid, flags, value);
		case HXB_DTYPE_128STRING:
			std::cout << "Sending value " << value << std::endl;
			return send_packet(net, ip, ValuePacket<std::string>(eid, value));

		case HXB_DTYPE_65BYTES:
		case HXB_DTYPE_16BYTES:
			std::cerr << "can't send binary data" << std::endl;
			return ERR_PARAMETER_VALUE_INVALID;

		case HXB_DTYPE_UNDEFINED:
			std::cerr << "unknown datatype" << std::endl;
			return ERR_PARAMETER_VALUE_INVALID;
		}
	} catch (const boost::bad_lexical_cast& e) {
		std::cerr << "Error while reading value: " << e.what() << std::endl;
		return ERR_PARAMETER_VALUE_INVALID;
	}
}

template<typename Content, typename ParsedType>
ErrorCode send_parsed_prop(hexabus::Socket& sock, const boost::asio::ip::address_v6& ip, uint32_t propid,
		uint32_t eid, uint8_t flags, const std::string& valueStr)
{
	ParsedType value = boost::lexical_cast<ParsedType>(valueStr);

	std::cout << "Sending value " << value << std::endl;
	return send_packet(sock, ip, hexabus::PropertyWritePacket<Content>(propid, eid, value, flags));
}

ErrorCode send_prop_write_packet(hexabus::Socket& net, const boost::asio::ip::address_v6& ip, uint32_t propid, uint32_t eid,
		uint8_t dt, uint8_t flags, const std::string& value)
{
	try {
		switch ((hxb_datatype) dt) {
		case HXB_DTYPE_BOOL: return send_parsed_prop<bool, bool>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_UINT8: return send_parsed_prop<uint8_t, unsigned>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_UINT16: return send_parsed_prop<uint16_t, uint16_t>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_UINT32: return send_parsed_prop<uint32_t, uint32_t>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_UINT64: return send_parsed_prop<uint64_t, uint64_t>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_SINT8: return send_parsed_prop<int8_t, int>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_SINT16: return send_parsed_prop<int16_t, int16_t>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_SINT32: return send_parsed_prop<int32_t, int32_t>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_SINT64: return send_parsed_prop<int64_t, int32_t>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_FLOAT: return send_parsed_prop<float, float>(net, ip, propid, eid, flags, value);
		case HXB_DTYPE_128STRING:
			std::cout << "Sending value " << value << std::endl;
			return send_packet(net, ip, PropertyWritePacket<std::string>(propid, eid, value));

		case HXB_DTYPE_65BYTES:
		case HXB_DTYPE_16BYTES:
			std::cerr << "can't send binary data" << std::endl;
			return ERR_PARAMETER_VALUE_INVALID;

		case HXB_DTYPE_UNDEFINED:
			std::cerr << "unknown datatype" << std::endl;
			return ERR_PARAMETER_VALUE_INVALID;
		}
	} catch (const boost::bad_lexical_cast& e) {
		std::cerr << "Error while reading value: " << e.what() << std::endl;
		return ERR_PARAMETER_VALUE_INVALID;
	}
}



enum Command {
	C_GET,
	C_SET,
	C_EPQUERY,
	C_SEND,
	C_LISTEN,
	C_ON,
	C_OFF,
	C_STATUS,
	C_POWER,
	C_DEVINFO,
	C_PROPQUERY,
	C_PROPWRITE
};


inline std::string shortDatatypeName(hxb_datatype type)
{
	switch (type) {
		case HXB_DTYPE_BOOL: return "b";
		case HXB_DTYPE_UINT8: return "u8";
		case HXB_DTYPE_UINT16: return "u16";
		case HXB_DTYPE_UINT32: return "u32";
		case HXB_DTYPE_UINT64: return "u65";
		case HXB_DTYPE_SINT8: return "s8";
		case HXB_DTYPE_SINT16: return "s16";
		case HXB_DTYPE_SINT32: return "s32";
		case HXB_DTYPE_SINT64: return "s64";
		case HXB_DTYPE_FLOAT: return "f";
		case HXB_DTYPE_128STRING: return "s";
		default: return "(unknown)";
	}
}

int dtypeStrToDType(const std::string& s)
{
	for(hxb_datatype  type = HXB_DTYPE_BOOL; type <= HXB_DTYPE_128STRING; type = hxb_datatype(type + 1)) {
		if(shortDatatypeName(type) == s || datatypeName(type) == s) {
			return type;
		}
	}
	return -1;
}

int main(int argc, char** argv) {

  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " ACTION IP [additional options] ";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexabus version and exit")
    ("command,c", po::value<std::string>(), "{get|set|epquery|propquery|propwrite|send|listen|on|off|status|power|devinfo}")
    ("ip,i", po::value<std::string>(), "the hostname to connect to")
    ("bind,b", po::value<std::string>(), "local IP address to use")
    ("interface,I", po::value<std::vector<std::string> >(), "for listen: interface to listen on. otherwise: outgoing interface for multicasts")
    ("eid,e", po::value<uint32_t>(), "Endpoint ID (EID)")
    ("datatype,d", po::value<std::string>(), "{u8|UInt8|u16|UInt16|u32|UInt32|u64|UInt64|s8|Int8|s16|Int16|s32|Int32|s64|Int64|f|Float|s|String|b|Bool}")
    ("propid,p", po::value<uint32_t>(), "Property ID")
    ("value,v", po::value<std::string>(), "Value")
    ("reliable,r", po::bool_switch(), "Reliable initialization of network access (adds delay, only needed for broadcasts)")
    ("oneline", po::bool_switch(), "Print each receive packet on one line")
    ("json", po::bool_switch(), "Print packet data as JSON, one packet per line")
    ;
  po::positional_options_description p;
  p.add("command", 1);
  p.add("ip", 1);
  po::variables_map vm;

  // Begin processing of commandline parameters.
  try {
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
		return ERR_UNKNOWN_PARAMETER;
  }

	Command command;
	boost::optional<boost::asio::ip::address_v6> ip;
	std::vector<std::string> interface;
  boost::asio::ip::address_v6 bind_addr(boost::asio::ip::address_v6::any());

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

	oneline = vm.count("oneline") && vm["oneline"].as<bool>();
	json = vm.count("json") && vm["json"].as<bool>();
	if (vm.count("interface")) {
		interface = vm["interface"].as<std::vector<std::string> >();
	}

  if (vm.count("version")) {
    std::cout << "hexaswitch -- command line hexabus client" << std::endl;
    std::cout << "libhexabus version " << hexabus::version() << std::endl;
    return 0;
  }

  if (! vm.count("command")) {
    std::cerr << "You must specify a command." << std::endl;
    return ERR_PARAMETER_MISSING;
  } else {
		std::string command_str = vm["command"].as<std::string>();

		if (boost::iequals(command_str, "GET")) {
			command = C_GET;
		} else if (boost::iequals(command_str, "SET")) {
			command = C_SET;
		} else if (boost::iequals(command_str, "EPQUERY")) {
			command = C_EPQUERY;
		} else if (boost::iequals(command_str, "SEND")) {
			command = C_SEND;
		} else if (boost::iequals(command_str, "LISTEN")) {
			command = C_LISTEN;
		} else if (boost::iequals(command_str, "ON")) {
			command = C_ON;
		} else if (boost::iequals(command_str, "OFF")) {
			command = C_OFF;
		} else if (boost::iequals(command_str, "STATUS")) {
			command = C_STATUS;
		} else if (boost::iequals(command_str, "POWER")) {
			command = C_POWER;
		} else if (boost::iequals(command_str, "DEVINFO")) {
			command = C_DEVINFO;
		} else if (boost::iequals(command_str, "PROPQUERY")) {
			command = C_PROPQUERY;
		} else if (boost::iequals(command_str, "PROPWRITE")) {
			command = C_PROPWRITE;
		} else {
			std::cerr << "Unknown command \"" << command_str << "\"" << std::endl;
			return ERR_PARAMETER_VALUE_INVALID;
		}
	}

	boost::asio::io_service io;

	if (vm.count("ip")) {
		boost::system::error_code err;
		ip = hexabus::resolve(io, vm["ip"].as<std::string>(), err);
		if (err) {
			std::cerr << vm["ip"].as<std::string>() << " is not a valid IP address: " << err.message() << std::endl;
			return ERR_PARAMETER_VALUE_INVALID;
		}
	}

  if (vm.count("bind")) {
		boost::system::error_code err;
    bind_addr = hexabus::resolve(io, vm["bind"].as<std::string>(), err);
		if (err) {
			std::cerr << vm["bind"].as<std::string>() << " is not a valid IP address: " << err.message() << std::endl;
			return ERR_PARAMETER_VALUE_INVALID;
		}
    std::cout << "Binding to " << bind_addr << std::endl;
  }

	if (command == C_LISTEN && !interface.size()) {
		std::cerr << "Command LISTEN requires an interface to listen on." << std::endl;
		return ERR_PARAMETER_MISSING;
	}

	hexabus::Listener listener(io);
	hexabus::Socket socket(io);

	if (interface.size()) {
		if (!json)
			std::cout << "Using interface " << interface[0] << std::endl;

		try {
			socket.mcast_from(interface[0]);
		} catch (const hexabus::NetworkException& e) {
			std::cerr << "Could not open socket on interface " << interface[0] << ": " << e.code().message() << std::endl;
			return ERR_NETWORK;
		}
	}

	if (command == C_LISTEN) {
		std::stringstream tmp;
		std::ostream& lbuf = json ? tmp : std::cout;

		lbuf << "Entering listen mode on";

		for (std::vector<std::string>::const_iterator it = interface.begin(), end = interface.end();
				it != end;
				it++) {
			lbuf << " " << *it;
			try {
				listener.listen(*it);
			} catch (const hexabus::NetworkException& e) {
				std::cerr << "Cannot listen on " << *it << ": " << e.what() << "; " << e.code().message() << std::endl;
				return ERR_NETWORK;
			}
		}
		lbuf << std::endl;

		while (true) {
			std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> pair;
			try {
				pair = listener.receive();
			} catch (const hexabus::NetworkException& e) {
				std::cerr << "Error receiving packet: " << e.code().message() << std::endl;
				return ERR_NETWORK;
			} catch (const hexabus::GenericException& e) {
				std::cerr << "Error receiving packet: " << e.what() << std::endl;
				return ERR_NETWORK;
			}

			PacketPrinter(std::cout).printPacket(*pair.first, pair.second);
		}
	}

	if (!ip && command == C_SEND) {
		ip = hexabus::Socket::GroupAddress;
	} else if (!ip) {
		std::cerr << "Command requires an IP address" << std::endl;
		return ERR_PARAMETER_MISSING;
	}
	if (ip->is_multicast() && !interface.size()) {
		std::cerr << "Sending to multicast requires an interface to send from" << std::endl;
		return ERR_PARAMETER_MISSING;
	}

	try {
		socket.bind(bind_addr);
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Cannot bind to " << bind_addr << ": " << e.code().message() << std::endl;
		return ERR_NETWORK;
	}

	hexabus::LivenessReporter* liveness =
    vm.count("reliable") && vm["reliable"].as<bool>()
    ? new hexabus::LivenessReporter(socket)
    : 0;
	if (liveness) {
		liveness->establishPaths(1);
		liveness->start();
	}

	namespace hf = hexabus::filtering;

	if (ip->is_multicast() && (command == C_STATUS || command == C_POWER
				|| command == C_GET || command == C_EPQUERY || command == C_DEVINFO || command == C_PROPQUERY)) {
		std::cerr << "Cannot query all devices at once, query them individually." << std::endl;
		return ERR_PARAMETER_VALUE_INVALID;
	}

	switch (command) {
		case C_ON:
		case C_OFF:
			return send_packet(socket, *ip, hexabus::WritePacket<bool>(EP_POWER_SWITCH, command == C_ON));

		case C_STATUS:
			return send_packet_wait(socket, *ip, hexabus::QueryPacket(EP_POWER_SWITCH, hexabus::Packet::want_ack), hf::eid() == EP_POWER_SWITCH && hf::sourceIP() == *ip);

		case C_POWER:
			return send_packet_wait(socket, *ip, hexabus::QueryPacket(EP_POWER_METER, hexabus::Packet::want_ack), hf::eid() == EP_POWER_METER && hf::sourceIP() == *ip);

		case C_SET:
		case C_SEND:
			{
				if (!vm.count("eid")) {
					std::cerr << "Command needs an EID" << std::endl;
					return ERR_PARAMETER_MISSING;
				}
				if (!vm.count("datatype")) {
					std::cerr << "Command needs a data type" << std::endl;
					return ERR_PARAMETER_MISSING;
				}
				if (!vm.count("value")) {
					std::cerr << "Command needs a value" << std::endl;
					return ERR_PARAMETER_MISSING;
				}

				try {
					uint32_t eid = vm["eid"].as<uint32_t>();
					int dtype = dtypeStrToDType(vm["datatype"].as<std::string>());
					std::string value = vm["value"].as<std::string>();

					if (dtype < 0) {
						std::cerr << "unknown datatype " << vm["datatype"].as<std::string>() << std::endl;
						return ERR_PARAMETER_VALUE_INVALID;
					}

					if (command == C_SET) {
						return send_value_packet<hexabus::WritePacket>(socket, *ip, eid, dtype, hexabus::Packet::want_ack, value);
					} else {
						return send_value_packet<hexabus::InfoPacket>(socket, *ip, eid, dtype, hexabus::Packet::want_ack, value);
					}
				} catch (const std::exception& e) {
					std::cerr << "Cannot process option: " << e.what() << std::endl;
					return ERR_UNKNOWN_PARAMETER;
				}
			}

		case C_GET:
		case C_EPQUERY:
			{
				if (!vm.count("eid")) {
					std::cerr << "Command needs an EID" << std::endl;
					return ERR_PARAMETER_MISSING;
				}
				uint32_t eid = vm["eid"].as<uint32_t>();

				if (command == C_GET) {
					return send_packet_wait(socket, *ip, hexabus::QueryPacket(eid, hexabus::Packet::want_ack), hf::eid() == eid && hf::sourceIP() == *ip);
				} else {
					return send_packet_wait(socket, *ip, hexabus::EndpointQueryPacket(eid, hexabus::Packet::want_ack), hf::eid() == eid && hf::sourceIP() == *ip);
				}
			}

		case C_DEVINFO:
			return send_packet_wait(socket, *ip, hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR, hexabus::Packet::want_ack), hf::eid() == EP_DEVICE_DESCRIPTOR && hf::sourceIP() == *ip);

		case C_PROPQUERY:
			{
				if (!vm.count("eid")) {
					std::cerr << "Command needs an EID" << std::endl;
					return ERR_PARAMETER_MISSING;
				}
				if (!vm.count("propid")) {
					std::cerr << "Command needs a property ID" << std::endl;
					return ERR_PARAMETER_MISSING;
				}
				uint32_t eid = vm["eid"].as<uint32_t>();
				uint32_t propid = vm["propid"].as<uint32_t>();

				return send_packet_wait(socket, *ip, hexabus::PropertyQueryPacket(propid, eid, hexabus::Packet::want_ack), hf::eid() == eid && hf::sourceIP() == *ip);
			}
		case C_PROPWRITE:
			{
				if (!vm.count("eid")) {
					std::cerr << "Command needs an EID" << std::endl;
					return ERR_PARAMETER_MISSING;
				}
				if (!vm.count("propid")) {
					std::cerr << "Command needs a poperty ID" << std::endl;
					return ERR_PARAMETER_MISSING;
				}
				if (!vm.count("datatype")) {
					std::cerr << "Command needs a data type" << std::endl;
					return ERR_PARAMETER_MISSING;
				}
				if (!vm.count("value")) {
					std::cerr << "Command needs a value" << std::endl;
					return ERR_PARAMETER_MISSING;
				}

				try {
					uint32_t eid = vm["eid"].as<uint32_t>();
					uint32_t propid = vm["propid"].as<uint32_t>();
					unsigned int dtype = dtypeStrToDType(vm["datatype"].as<std::string>());
					std::string value = vm["value"].as<std::string>();

					return send_prop_write_packet(socket, *ip, propid, eid, dtype, 0, value);

				} catch (const std::exception& e) {
					std::cerr << "Cannot process option: " << e.what() << std::endl;
					return ERR_UNKNOWN_PARAMETER;
				}
			}
		default:
			std::cerr << "BUG: Unknown command" << std::endl;
			return ERR_OTHER;
	}

	return ERR_NONE;
}

