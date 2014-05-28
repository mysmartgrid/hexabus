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
#include "resolv.hpp"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

enum ErrorCode {
	ERR_NONE = 0,

	ERR_UNKNOWN_PARAMETER = 1,
	ERR_PARAMETER_MISSING = 2,
	ERR_PARAMETER_FORMAT = 3,
	ERR_PARAMETER_VALUE_INVALID = 4,
	ERR_NETWORK = 5,

	ERR_OTHER = 127
};

static bool oneline = false;

struct err_callback {
	boost::asio::io_service* io;
	ErrorCode err;
	void operator()(const hexabus::NetworkException& e)
	{
		std::cerr << "Error receiving packet: " << e.code().message() << std::endl;
		err = ERR_NETWORK;
		io->stop();
	}
	void operator()(const hexabus::GenericException& e)
	{
		std::cerr << "Error receiving packet: " << e.what() << std::endl;
		err = ERR_NETWORK;
		io->stop();
	}
};

struct PacketPrinter : public hexabus::PacketVisitor {
	private:
		std::ostream& target;

		void printValueHeader(uint32_t eid, const char* datatypeStr)
		{
			if (oneline) {
				target << "Info;EID " << eid << ";Datatype " << datatypeStr << ";";
			} else {
				target << "Info" << std::endl
					<< "Endpoint ID:\t" << eid << std::endl
					<< "Datatype:\t" << datatypeStr << std::endl;
			}
		}

		void printReportHeader(uint32_t eid, const char* datatypeStr, uint16_t cause)
		{
			if (oneline) {
				target << "Report;Cause " << cause << ";EID " << eid << ";Datatype " << datatypeStr << ";";
			} else {
				target << "Report" << std::endl
					<< "Cause:\t" << cause << std::endl
					<< "Endpoint ID:\t" << eid << std::endl
					<< "Datatype:\t" << datatypeStr << std::endl;
			}
		}

		void printProxyInfoHeader(const boost::asio::ip::address_v6& source, uint32_t eid, const char* datatypeStr)
		{
			if (oneline) {
				target << "ProxyInfo;Origin " << source << ";EID " << eid << ";Datatype " << datatypeStr << ";";
			} else {
				target << "ProxyInfo" << std::endl
					<< "Origin:\t" << source << std::endl
					<< "Endpoint ID:\t" << eid << std::endl
					<< "Datatype:\t" << datatypeStr << std::endl;
			}
		}

		template<typename T>
		void printValue(const hexabus::ValuePacket<T>& packet)
		{
			if (oneline) {
				target << "Value " << packet.value();
			} else {
				target << "Value:\t" << packet.value() << std::endl;
			}
			target << std::endl;
		}

		void printValue(const hexabus::ValuePacket<uint8_t>& packet)
		{
			if (oneline) {
				target << "Value " << (int) packet.value();
			} else {
				target << "Value:\t" << (int) packet.value() << std::endl;
			}
			target << std::endl;
		}

		template<size_t L>
		void printValue(const hexabus::ValuePacket<boost::array<char, L> >& packet)
		{
			std::stringstream hexstream;

			hexstream << std::hex << std::setfill('0');

			for (size_t i = 0; i < L; ++i) {
				hexstream << std::setw(2) << (0xFF & packet.value()[i]) << " ";
			}

			std::cout << std::endl << std::endl;

			if (oneline) {
				target << "Value " << hexstream.str();
			} else {
				target << "Value:\t" << hexstream.str() << std::endl;
			}
			target << std::endl;
		}

		template<typename TValue>
		void printValuePacket(const hexabus::InfoPacket<TValue>& info, const char* datatypeStr)
		{
			printValueHeader(info.eid(), datatypeStr);
			printValue(info);
		}

		template<typename TInfo>
		void printReportPacket(const hexabus::ReportPacket<TInfo>& report, const char* datatypeStr)
		{
			printReportHeader(report.eid(), datatypeStr, report.cause());
			printValue(report);
		}

		template<typename TInfo>
		void printProxyInfoPacket(const hexabus::ProxyInfoPacket<TInfo>& proxyInfo, const char* datatypeStr)
		{
			printProxyInfoHeader(proxyInfo.origin(), proxyInfo.eid(), datatypeStr);
			printValue(proxyInfo);
		}

		void printEndpointHeader(const hexabus::EndpointInfoPacket& endpointInfo)
		{
			if (endpointInfo.eid() % 32 == 0) {
				if (oneline) {
					target << "Device Info;Device Name " << endpointInfo.value();
				} else {
					target << "Device Info" << std::endl
						<< "Device Name:\t" << endpointInfo.value() << std::endl;
				}
			} else {
				if (oneline) {
					target << "Endpoint Info;EID " << endpointInfo.eid() << ";Datatype ";
				} else {
					target << "Endpoint Info\n"
						<< "Endpoint ID:\t" << endpointInfo.eid() << std::endl
						<< "EP Datatype:\t";
				}
			}
		}

		void printEndpointHeader(const hexabus::EndpointReportPacket& endpointInfo)
		{
			if (endpointInfo.eid() % 32 == 0) {
				if (oneline) {
					target << "Device Info;Cause " << endpointInfo.cause() << ";Device Name " << endpointInfo.value();
				} else {
					target << "Device Info" << std::endl
						<< "Cause:\t" << endpointInfo.cause() << std::endl
						<< "Device Name:\t" << endpointInfo.value() << std::endl;
				}
			} else {
				if (oneline) {
					target << "Endpoint Info;Cause " << endpointInfo.cause() << ";EID " << endpointInfo.eid() << ";Datatype ";
				} else {
					target << "Endpoint Info\n"
						<< "Cause:\t" << endpointInfo.cause() << std::endl
						<< "Endpoint ID:\t" << endpointInfo.eid() << std::endl
						<< "EP Datatype:\t";
				}
			}
		}

		void printTimeInfo(const hexabus::TimeInfoPacket& timeinfo)
		{
			if(oneline) {
				target << "Time Info; "<< timeinfo.datetime() << "; ";
			} else {
				target << "Time Info\n"
					<< "Time:\t" << timeinfo.datetime();
			}
		}

		template<typename Packet>
		void printEndpointInfo(const Packet& endpointInfo)
		{
			printEndpointHeader(endpointInfo);
			if (endpointInfo.eid() % 32 != 0) {
				switch(endpointInfo.datatype()) {
					case HXB_DTYPE_BOOL:
						target << "Bool";
						break;
					case HXB_DTYPE_UINT8:
						target << "UInt8";
						break;
					case HXB_DTYPE_UINT32:
						target << "UInt32";
						break;
					case HXB_DTYPE_FLOAT:
						target << "Float";
						break;
					case HXB_DTYPE_128STRING:
						target << "String";
						break;
					case HXB_DTYPE_16BYTES:
						target << "Binary (16bytes)";
						break;
					case HXB_DTYPE_66BYTES:
						target << "Binary (66bytes)";
						break;
					default:
						target << "(unknown)";
						break;
				}
				if (oneline) {
					target << ";Name " << endpointInfo.value();
				} else {
					target << std::endl;
					target << "EP Name:\t" << endpointInfo.value() << std::endl;
				}
			}
			target << std::endl;
		}

	public:
		PacketPrinter(std::ostream& target)
			: target(target)
		{}

		virtual void visit(const hexabus::ErrorPacket& error)
		{
			if (oneline) {
				target << "Error;Error Code ";
			} else {
				target << "Error" << std::endl
					<< "Error code:\t";
			}
			switch (error.code()) {
				case HXB_ERR_UNKNOWNEID:
					target << "Unknown EID";
					break;
				case HXB_ERR_WRITEREADONLY:
					target << "Write on readonly endpoint";
					break;
				case HXB_ERR_CRCFAILED:
					target << "CRC failed";
					break;
				case HXB_ERR_DATATYPE:
					target << "Datatype mismatch";
					break;
				case HXB_ERR_INVALID_VALUE:
					target << "Invalid value";
					break;
				default:
					target << "(unknown: " <<  static_cast<unsigned int>(error.code()) << ")";
					break;
			}
			target << std::endl;
		}

		virtual void visit(const hexabus::QueryPacket& query) {}
		virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) {}

		virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo) { printEndpointInfo(endpointInfo); }
		virtual void visit(const hexabus::EndpointReportPacket& endpointReport) { printEndpointInfo(endpointReport); }

		virtual void visit(const hexabus::AckPacket& ack) {}

		virtual void visit(const hexabus::TimeInfoPacket& timeinfo) { printTimeInfo(timeinfo); }

		virtual void visit(const hexabus::InfoPacket<bool>& info) { printValuePacket(info, "Bool"); }
		virtual void visit(const hexabus::InfoPacket<uint8_t>& info) { printValuePacket(info, "UInt8"); }
		virtual void visit(const hexabus::InfoPacket<uint32_t>& info) { printValuePacket(info, "UInt32"); }
		virtual void visit(const hexabus::InfoPacket<float>& info) { printValuePacket(info, "Float"); }
		virtual void visit(const hexabus::InfoPacket<std::string>& info) { printValuePacket(info, "String"); }
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (16 bytes)"); }
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (66 bytes)"); }

		virtual void visit(const hexabus::ReportPacket<bool>& report) { printReportPacket(report, "Bool"); }
		virtual void visit(const hexabus::ReportPacket<uint8_t>& report) { printReportPacket(report, "UInt8"); }
		virtual void visit(const hexabus::ReportPacket<uint32_t>& report) { printReportPacket(report, "UInt32"); }
		virtual void visit(const hexabus::ReportPacket<float>& report) { printReportPacket(report, "Float"); }
		virtual void visit(const hexabus::ReportPacket<std::string>& report) { printReportPacket(report, "String"); }
		virtual void visit(const hexabus::ReportPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& report) { printReportPacket(report, "Binary (16 bytes)"); }
		virtual void visit(const hexabus::ReportPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& report) { printReportPacket(report, "Binary (66 bytes)"); }

		virtual void visit(const hexabus::ProxyInfoPacket<bool>& proxyInfo) { printProxyInfoPacket(proxyInfo, "Bool"); }
		virtual void visit(const hexabus::ProxyInfoPacket<uint8_t>& proxyInfo) { printProxyInfoPacket(proxyInfo, "UInt8"); }
		virtual void visit(const hexabus::ProxyInfoPacket<uint32_t>& proxyInfo) { printProxyInfoPacket(proxyInfo, "UInt32"); }
		virtual void visit(const hexabus::ProxyInfoPacket<float>& proxyInfo) { printProxyInfoPacket(proxyInfo, "Float"); }
		virtual void visit(const hexabus::ProxyInfoPacket<std::string>& proxyInfo) { printProxyInfoPacket(proxyInfo, "String"); }
		virtual void visit(const hexabus::ProxyInfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& proxyInfo) { printProxyInfoPacket(proxyInfo, "Binary (16 bytes)"); }
		virtual void visit(const hexabus::ProxyInfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& proxyInfo) { printProxyInfoPacket(proxyInfo, "Binary (66 bytes)"); }

		virtual void visit(const hexabus::WritePacket<bool>& write) {}
		virtual void visit(const hexabus::WritePacket<uint8_t>& write) {}
		virtual void visit(const hexabus::WritePacket<uint32_t>& write) {}
		virtual void visit(const hexabus::WritePacket<float>& write) {}
		virtual void visit(const hexabus::WritePacket<std::string>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
};

void print_packet(const hexabus::Packet& packet)
{
	PacketPrinter pp(std::cout);

	pp.visitPacket(packet);
}

struct send_packet_trn_callback {
	boost::asio::io_service* io;
	ErrorCode err;

	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep, bool failed)
	{
		io->stop();
		if(failed) {
			std::cout<<"Ack timeout!"<<std::endl;
			err = ERR_NETWORK;
		}
	}
};

ErrorCode send_packet(hexabus::Socket& net, const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet)
{
	send_packet_trn_callback tcb = {&net.ioService(), ERR_NONE};

	net.onPacketTransmitted(boost::ref(tcb), packet, boost::asio::ip::udp::endpoint(addr, HXB_PORT));

	net.ioService().reset();
	net.ioService().run();

	return tcb.err;
}

struct send_packet_wait_rcv_callback {
	boost::asio::io_service* io;
	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep)
	{
		print_packet(packet);
		io->stop();
	}
};

struct send_packet_wait_trn_callback {
	boost::asio::io_service* io;
	ErrorCode err;

	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep, bool failed)
	{
		if(failed) {
			std::cerr<<"Ack timeout!"<<std::endl;
			err = ERR_NETWORK;
			io->stop();
		}
	}
};

template<typename Filter>
ErrorCode send_packet_wait(hexabus::Socket& net, const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet, const Filter& filter)
{
	send_packet_wait_trn_callback tcb = {&net.ioService(), ERR_NONE};

	net.onPacketTransmitted(boost::ref(tcb), packet, boost::asio::ip::udp::endpoint(addr, HXB_PORT));

	send_packet_wait_rcv_callback rcb = {&net.ioService()};
	err_callback ecb = {&net.ioService(), ERR_NONE};

	namespace hf = hexabus::filtering;
	boost::signals2::connection rec_con = net.onPacketReceived(rcb, (filter || hf::isError()) && hf::sourceIP() == addr);
	boost::signals2::connection err_con = net.onAsyncError(boost::ref(ecb));

	net.ioService().reset();
	net.ioService().run();

	rec_con.disconnect();
	err_con.disconnect();

	return tcb.err?tcb.err:ecb.err;
}

template<template<typename TValue> class ValuePacket>
ErrorCode send_value_packet(hexabus::Socket& net, const boost::asio::ip::address_v6& ip, uint32_t eid, uint8_t datatype, const std::string& value)
{
	try { // handle errors in value lexical_cast
		switch (datatype) {
			case HXB_DTYPE_BOOL:
				{
					bool b = boost::lexical_cast<unsigned int>(value);
					std::cout << "Sending value " << b << std::endl;
					return send_packet(net, ip, ValuePacket<bool>(eid, b, hexabus::Packet::want_ack));
				}

			case HXB_DTYPE_UINT8:
				{
					uint8_t u8 = boost::lexical_cast<unsigned int>(value);
					std::cout << "Sending value " << (unsigned int) u8 << std::endl;
					return send_packet(net, ip, ValuePacket<uint8_t>(eid, u8, hexabus::Packet::want_ack));
				}

			case HXB_DTYPE_UINT32:
				{
					uint32_t u32 = boost::lexical_cast<uint32_t>(value);
					std::cout << "Sending value " << u32 << std::endl;
					return send_packet(net, ip, ValuePacket<uint32_t>(eid, u32, hexabus::Packet::want_ack));
				}

			case HXB_DTYPE_FLOAT:
				{
					float f = boost::lexical_cast<float>(value);
					std::cout << "Sending value " << f << std::endl;
					return send_packet(net, ip, ValuePacket<float>(eid, f, hexabus::Packet::want_ack));
				}

			case HXB_DTYPE_128STRING:
				{
					std::cout << "Sending value " << value << std::endl;
					return send_packet(net, ip, ValuePacket<std::string>(eid, value, hexabus::Packet::want_ack));
				}

			default:
				std::cout << "Unknown data type " << datatype << std::endl;
				return ERR_PARAMETER_VALUE_INVALID;
		}
	} catch (boost::bad_lexical_cast& e) {
		std::cerr << "Error while converting value: " << e.what() << std::endl;
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

struct listener_rcv_callback {
	boost::asio::io_service* io;
	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep)
	{
		if (oneline) {
			std::cout << "From " << asio_ep << ";";
		} else {
			std::cout << "Received packet from " << asio_ep << std::endl;
		}
		print_packet(packet);
	}
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
    ("datatype,d", po::value<unsigned int>(), "{1: Bool | 2: UInt8 | 3: UInt32 | 5: Float | 6: String}")
    ("value,v", po::value<std::string>(), "Value")
    ("reliable,r", po::bool_switch(), "Reliable initialization of network access (adds delay, only needed for broadcasts)")
    ("oneline", po::bool_switch(), "Print each receive packet on one line")
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
		std::cout << "Using interface " << interface[0] << std::endl;

		try {
			socket.mcast_from(interface[0]);
		} catch (const hexabus::NetworkException& e) {
			std::cerr << "Could not open socket on interface " << interface[0] << ": " << e.code().message() << std::endl;
			return ERR_NETWORK;
		}
	}

	if (command == C_LISTEN) {
		std::cout << "Entering listen mode on";

		for (std::vector<std::string>::const_iterator it = interface.begin(), end = interface.end();
				it != end;
				it++) {
			std::cout << " " << *it;
			try {
				listener.listen(*it);
			} catch (const hexabus::NetworkException& e) {
				std::cerr << "Cannot listen on " << *it << ": " << e.what() << "; " << e.code().message() << std::endl;
				return ERR_NETWORK;
			}
		}
		std::cout << std::endl;

		while (true) {
			listener_rcv_callback rcb = {&listener.ioService()};
			err_callback ecb = {&listener.ioService(), ERR_NONE};

			boost::signals2::connection rec_con = listener.onPacketReceived(rcb);
			boost::signals2::connection err_con = listener.onAsyncError(boost::ref(ecb));

			listener.ioService().run();

			rec_con.disconnect();
			err_con.disconnect();

			if(ecb.err)
				return ecb.err;
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
					unsigned int dtype = vm["datatype"].as<unsigned int>();
					std::string value = vm["value"].as<std::string>();

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
					return send_packet_wait(socket, *ip, hexabus::QueryPacket(eid, hexabus::Packet::want_ack), hf::eid() == eid && hf::sourceIP() == *ip);
				} else {
					return send_packet_wait(socket, *ip, hexabus::EndpointQueryPacket(eid, hexabus::Packet::want_ack), hf::eid() == eid && hf::sourceIP() == *ip);
				}
			}

		case C_DEVINFO:
			return send_packet_wait(socket, *ip, hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR, hexabus::Packet::want_ack), hf::eid() == EP_DEVICE_DESCRIPTOR && hf::sourceIP() == *ip);

		default:
			std::cerr << "BUG: Unknown command" << std::endl;
			return ERR_OTHER;
	}

	return ERR_NONE;
}
