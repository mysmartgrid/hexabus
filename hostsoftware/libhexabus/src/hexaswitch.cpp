#include <iostream>
#include <string.h>
#include <iomanip>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
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
	F_EID,
	F_DATATYPE,
	F_TYPE,
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
		case F_EID: fieldNameStr = "EID"; break;
		case F_DATATYPE: fieldNameStr = "Datatype"; break;
		case F_TYPE: fieldNameStr = "Type"; break;
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
	void addField(FieldName name, const boost::array<char, L>& value)
	{
		beginField(name);

		unsigned printed = 0;
		for (auto c : value) {
			if (printed++)
				buffer << ", ";

			buffer << "0x" << std::hex << std::setw(2) << unsigned(int(c));
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
		case F_EID: fieldNameStr = "eid"; break;
		case F_DATATYPE: fieldNameStr = "datatype"; break;
		case F_TYPE: fieldNameStr = "type"; break;
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
	void addField(FieldName name, const boost::array<char, L>& value)
	{
		beginField(name);

		buffer << '[';

		unsigned printed = 0;
		for (auto c : value) {
			if (printed++)
				buffer << ", ";

			buffer << "0x" << std::hex << std::setw(2) << unsigned(int(c));
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

	virtual void visit(const hexabus::ErrorPacket& error)
	{
		buffer.addField(F_TYPE, "Error");
		buffer.addField(F_ERROR_CODE, unsigned(error.code()));
		buffer.addField(F_ERROR_STR, errcodeStr(error.code()));
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

public:
	template<typename... Args>
	BufferedPrinter(std::ostream& target, Args... args)
		: target(target), buffer(std::forward<Args>(args)...)
	{}

	virtual void printPacket(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
	{
		buffer.addField(F_FROM, from.address().to_string());
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

ErrorCode send_packet(hexabus::Socket& net, const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet)
{
	try {
		net.send(packet, addr);
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Could not send packet to " << addr << ": " << e.code().message() << std::endl;
		return ERR_NETWORK;
	}

	return ERR_NONE;
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

template<template<typename TValue> class Packet, typename Content, typename Out>
ErrorCode send_parsed_value(hexabus::Socket& sock, const boost::asio::ip::address_v6& ip, uint32_t eid, const std::string& valueStr)
{
	Content value = boost::lexical_cast<Content>(valueStr);

	std::cout << "Sending value " << Out(value) << std::endl;
	return send_packet(sock, ip, Packet<Content>(eid, value));
}

template<template<typename TValue> class ValuePacket>
ErrorCode send_value_packet(hexabus::Socket& net, const boost::asio::ip::address_v6& ip, uint32_t eid, uint8_t dt,
		const std::string& value)
{
	try {
		switch ((hxb_datatype) dt) {
		case HXB_DTYPE_BOOL: return send_parsed_value<ValuePacket, bool, bool>(net, ip, eid, value);
		case HXB_DTYPE_UINT8: return send_parsed_value<ValuePacket, uint8_t, unsigned>(net, ip, eid, value);
		case HXB_DTYPE_UINT16: return send_parsed_value<ValuePacket, uint16_t, uint16_t>(net, ip, eid, value);
		case HXB_DTYPE_UINT32: return send_parsed_value<ValuePacket, uint32_t, uint32_t>(net, ip, eid, value);
		case HXB_DTYPE_UINT64: return send_parsed_value<ValuePacket, uint64_t, uint64_t>(net, ip, eid, value);
		case HXB_DTYPE_SINT8: return send_parsed_value<ValuePacket, int8_t, int>(net, ip, eid, value);
		case HXB_DTYPE_SINT16: return send_parsed_value<ValuePacket, int16_t, int16_t>(net, ip, eid, value);
		case HXB_DTYPE_SINT32: return send_parsed_value<ValuePacket, int32_t, int32_t>(net, ip, eid, value);
		case HXB_DTYPE_SINT64: return send_parsed_value<ValuePacket, int64_t, int32_t>(net, ip, eid, value);
		case HXB_DTYPE_FLOAT: return send_parsed_value<ValuePacket, float, float>(net, ip, eid, value);
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
	C_DEVINFO
};

int main(int argc, char** argv) {


  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " ACTION IP [additional options] ";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexabus version and exit")
    ("command,c", po::value<std::string>(), "{get|set|epquery|send|listen|on|off|status|power|devinfo}")
    ("ip,i", po::value<std::string>(), "the hostname to connect to")
    ("bind,b", po::value<std::string>(), "local IP address to use")
    ("interface,I", po::value<std::vector<std::string> >(), "for listen: interface to listen on. otherwise: outgoing interface for multicasts")
    ("eid,e", po::value<uint32_t>(), "Endpoint ID (EID)")
    ("datatype,d", po::value<std::string>(), "{u8|UInt8|u16|UInt16|u32|UInt32|u64|UInt64|s8|Int8|s16|Int16|s32|Int32|s64|Int64|f|Float|s|String|b|Bool}")
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
				|| command == C_GET || command == C_EPQUERY || command == C_DEVINFO)) {
		std::cerr << "Cannot query all devices at once, query them individually." << std::endl;
		return ERR_PARAMETER_VALUE_INVALID;
	}

	switch (command) {
		case C_ON:
		case C_OFF:
			return send_packet(socket, *ip, hexabus::WritePacket<bool>(EP_POWER_SWITCH, command == C_ON));

		case C_STATUS:
			return send_packet_wait(socket, *ip, hexabus::QueryPacket(EP_POWER_SWITCH), hf::eid() == EP_POWER_SWITCH && hf::sourceIP() == *ip);

		case C_POWER:
			return send_packet_wait(socket, *ip, hexabus::QueryPacket(EP_POWER_METER), hf::eid() == EP_POWER_METER && hf::sourceIP() == *ip);

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
						return send_value_packet<hexabus::WritePacket>(socket, *ip, eid, dtype, value);
					} else {
						return send_value_packet<hexabus::InfoPacket>(socket, *ip, eid, dtype, value);
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
					return send_packet_wait(socket, *ip, hexabus::QueryPacket(eid), hf::eid() == eid && hf::sourceIP() == *ip);
				} else {
					return send_packet_wait(socket, *ip, hexabus::EndpointQueryPacket(eid), hf::eid() == eid && hf::sourceIP() == *ip);
				}
			}

		case C_DEVINFO:
			return send_packet_wait(socket, *ip, hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR), hf::eid() == EP_DEVICE_DESCRIPTOR && hf::sourceIP() == *ip);

		default:
			std::cerr << "BUG: Unknown command" << std::endl;
			return ERR_OTHER;
	}

	return ERR_NONE;
}

