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
#include "shared.hpp"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

using namespace hexabus;

namespace {

enum ErrorCode {
	ERR_NONE = 0,

	ERR_UNKNOWN_PARAMETER = 1,
	ERR_PARAMETER_MISSING = 2,
	ERR_PARAMETER_FORMAT = 3,
	ERR_PARAMETER_VALUE_INVALID = 4,
	ERR_NETWORK = 5,

	ERR_OTHER = 127
};

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

class PlaintextBuffer {
private:
	std::stringstream buffer;
	bool oneline;

	void beginField(FieldName fieldName)
	{
		if (buffer.tellp())
			buffer << (oneline ? ';' : '\n');

		std::string fieldNameStr;
		switch (fieldName) {
		case F_FROM: fieldNameStr = "Packet from"; break;
		case F_VALUE: fieldNameStr = "Value"; break;
		case F_SEQNUM: name = "Sequence Number"; break;
		case F_EID: fieldNameStr = "EID"; break;
		case F_DATATYPE: fieldNameStr = "Datatype"; break;
		case F_TYPE: fieldNameStr = "Type"; break;
		case F_PROPID: name = "Property ID"; break;
		case F_NEXTPROP: name = "Next property ID"; break;
		case F_CAUSE: name = "Cause"; break;
		case F_ORIGIN: name = "Origin IP"; break;
		case F_ERROR_CODE: fieldNameStr = "Error code"; break;
		case F_ERROR_STR: fieldNameStr = "Error"; break;
		}

		buffer
			<< fieldNameStr
			<< std::string(oneline ? 1 : 12 - fieldNameStr.size(), ' ');
	}

public:
	PlaintextBuffer(bool oneline)
		: oneline(oneline)
	{}

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
				buffer << "\\x" << std::hex << std::setw(2) << unsigned(int(c));
		}
	}

	template<size_t L>
	void addField(FieldName name, const std::array<uint8_t, L>& value)
	{
		beginField(name);

		unsigned printed = 0;
		for (auto c : value) {
			if (printed++)
				buffer << ", ";

			buffer << "0x" << std::hex << std::setw(2) << unsigned(c);
		}
	}

	std::string finish()
	{
		auto result = buffer.str();
		buffer.str("");
		return result + (oneline ? "" : "\n");
	}
};

class JsonBuffer {
private:
	std::stringstream buffer;

	void beginField(FieldName fieldName)
	{
		if (buffer.tellp())
			buffer << ',';

		std::string fieldNameStr;
		switch (fieldName) {
		case F_FROM: fieldNameStr = "from"; break;
		case F_VALUE: fieldNameStr = "value"; break;
		case F_SEQNUM: fieldNameStr = "seq_num"; break;
		case F_EID: fieldNameStr = "eid"; break;
		case F_DATATYPE: fieldNameStr = "datatype"; break;
		case F_TYPE: fieldNameStr = "type"; break;
			case F_PROPID: fieldNameStr = "prop_id"; break;
		case F_NEXTPROP: fieldNameStr = "next_prop_id"; break;
		case F_CAUSE: fieldNameStr = "cause"; break;
		case F_ORIGIN: fieldNameStr = "origin"; break;
		case F_ERROR_CODE: fieldNameStr = "error_code"; break;
		case F_ERROR_STR: fieldNameStr = "error_str"; break;
		}

		buffer << '"' << fieldNameStr << '"' << ':';
	}

public:
	template<typename Value>
	void addField(FieldName name, const Value& value)
	{
		beginField(name);
		buffer << value;
	}

	void addField(FieldName name, const std::string& value)
	{
		beginField(name);

		buffer << '"';
		for (auto c : value) {
			if (std::isprint(c) && c != '"')
				buffer << c;
			else
				buffer << "\\x" << std::hex << std::setw(2) << unsigned(int(c));
		}
		buffer << '"';
	}

	template<size_t L>
	void addField(FieldName name, const std::array<uint8_t, L>& value)
	{
		beginField(name);

		buffer << '[';

		unsigned printed = 0;
		for (auto c : value) {
			if (printed++)
				buffer << ", ";

			buffer << "0x" << std::hex << std::setw(2) << unsigned(c);
		}

		buffer << ']';
	}

	std::string finish()
	{
		auto result = "{" + buffer.str() + "}";
		buffer.str("");
		return result;
	}
};

struct Printer {
	virtual void printPacket(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from) = 0;
	virtual void printPacket(const hexabus::Packet& p) = 0;
};

template<typename Buffer>
class BufferedPrinter : public Printer, private hexabus::PacketVisitor {
private:
	std::ostream& target;
	Buffer buffer;

	template<typename T>
	void printValuePacket(const hexabus::ValuePacket<T>& packet, const char* type)
	{
		buffer.addField(F_TYPE, type);
		buffer.addField(F_EID, packet.eid());
		buffer.addField(F_DATATYPE, hexabus::datatypeName((hexabus::hxb_datatype) packet.datatype()));

		typedef typename std::conditional<sizeof(T) == 1, int, const T&>::type Widened;
		buffer.addField(F_VALUE, Widened(packet.value()));
	}

	template<typename T>
	void printReportPacket(const hexabus::ReportPacket<T>& packet)
	{
		printValuePacket(packet, "Report");
		buffer.addField(F_CAUSE, packet.cause());
	}

	template<typename T>
	void printProxyInfoPacket(const hexabus::ProxyInfoPacket<T>& packet)
	{
		printValuePacket(packet, "PInfo");
		buffer.addField(F_ORIGIN, packet.origin().to_string());
	}

	template<typename T>
	void printPropertyWritePacket(const hexabus::PropertyWritePacket<T>& packet)
	{
		buffer.addField(F_PROPID, packet.propid());
		printValuePacket(packet, "PropWrite");
	}

	template<typename T>
	void printPropertyReportPacket(const hexabus::PropertyReportPacket<T>& packet)
	{
		printValuePacket(packet, "PropReport");
		buffer.addField(F_NEXTPROP, packet.nextid());
		buffer.addField(F_CAUSE, packet.cause());
	}

	virtual void visit(const hexabus::ErrorPacket& error)
	{
		buffer.addField(F_TYPE, "Error");
		buffer.addField(F_ERROR_CODE, unsigned(error.code()));
		buffer.addField(F_ERROR_STR, errcodeStr(error.code()));
		buffer.addField(F_CAUSE, error.cause());
	}

	virtual void visit(const hexabus::QueryPacket& query)
	{
		buffer.addField(F_TYPE, "Query");
		buffer.addField(F_EID, query.eid());
	}

	virtual void visit(const hexabus::EndpointQueryPacket& query)
	{
		buffer.addField(F_TYPE, "EPQuery");
		buffer.addField(F_EID, query.eid());
	}

	virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo)
	{
		buffer.addField(F_TYPE, "EPInfo");
		buffer.addField(F_EID, endpointInfo.eid());
		buffer.addField(F_DATATYPE, hexabus::datatypeName((hexabus::hxb_datatype) endpointInfo.datatype()));
		buffer.addField(F_VALUE, endpointInfo.value());
	}

	virtual void visit(const hexabus::EndpointReportPacket& endpointReport)
	{
		fields.addField(F_TYPE, "EPReport");
		fields.addField(F_EID, endpointReport.eid());
		fields.addField(F_DATATYPE, hexabus::datatypeName((hexabus::hxb_datatype) endpointReport.datatype()));
		fields.addField(F_VALUE, endpointReport.value());
		fields.addField(F_CAUSE, endpointReport.cause());
	}

	virtual void visit(const hexabus::AckPacket& ack)
	{
		fields.addField(F_TYPE, "Ack");
		fields.addField(F_CAUSE, ack.cause()));
	}

	virtual void visit(const hexabus::PropertyQueryPacket& propertyQuery)
	{
		fields.addField(F_TYPE, "PropQuery");
		fields.addField(F_EID, propertyQuery.eid());
		fields.addField(F_PROPID, propertyQuery.propid());
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
	template<typename... Args>
	BufferedPrinter(std::ostream& target, Args... args)
		: target(target), buffer(std::forward<Args>(args)...)
	{}

	virtual void printPacket(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
	{
		buffer.addField(F_FROM, from.address().to_string());
		buffer.addField(F_SEQNUM, p.sequenceNumber());
		p.accept(*this);
		target << buffer.finish() << std::endl;
	}

	virtual void printPacket(const hexabus::Packet& p)
	{
		p.accept(*this);
		target << buffer.finish() << std::endl;
	}
};



static std::unique_ptr<Printer> packetPrinter;

}

void print_packet(const hexabus::Packet& packet)
{
	packetPrinter->printPacket(packet);
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

	bool oneline = vm.count("oneline") && vm["oneline"].as<bool>();
	bool json = vm.count("json") && vm["json"].as<bool>();

	if (json)
		packetPrinter.reset(new BufferedPrinter<JsonBuffer>(std::cout));
	else
		packetPrinter.reset(new BufferedPrinter<PlaintextBuffer>(std::cout, oneline));

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

			packetPrinter->printPacket(*pair.first, pair.second);
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

