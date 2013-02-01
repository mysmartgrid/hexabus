#include <iostream>
#include <string.h>
#include <iomanip>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/liveness.hpp>
#include <time.h>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <libhexabus/socket.hpp>
#include <typeinfo>
namespace po = boost::program_options;

#include "../../../shared/hexabus_packet.h"

#pragma GCC diagnostic warning "-Wstrict-aliasing"


struct PacketPrinter : public hexabus::PacketVisitor {
	private:
		std::ostream& target;

		void printValueHeader(uint32_t eid, const char* datatypeStr)
		{
			target << "Info" << std::endl
				<< "Endpoint ID:\t" << eid << std::endl
				<< "Datatype:\t" << datatypeStr << std::endl;
		}

		template<typename T>
		void printValuePacket(const hexabus::ValuePacket<T>& packet, const char* datatypeStr)
		{
			printValueHeader(packet.eid(), datatypeStr);
			target << "Value:\t" << packet.value() << std::endl;
			target << std::endl;
		}

		void printValuePacket(const hexabus::ValuePacket<uint8_t>& packet, const char* datatypeStr)
		{
			printValueHeader(packet.eid(), datatypeStr);
			target << "Value:\t" << (int) packet.value() << std::endl;
			target << std::endl;
		}

    template<size_t L>
		void printValuePacket(const hexabus::ValuePacket<boost::array<char, L> >& packet, const char* datatypeStr)
		{
			printValueHeader(packet.eid(), datatypeStr);

			std::stringstream hexstream;

			hexstream << std::hex << std::setfill('0');

			for (size_t i = 0; i < L; ++i)
      {
				hexstream << std::setw(2) << (0xFF & packet.value()[i]) << " ";
			}

      std::cout << std::endl << std::endl;

			target << "Value:\t" << hexstream.str() << std::endl; 
			target << std::endl;
		}

	public:
		PacketPrinter(std::ostream& target)
			: target(target)
		{}

		virtual void visit(const hexabus::ErrorPacket& error)
		{
			target << "Error" << std::endl
				<< "Error code:\t";
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
					target << "(unknown)";
					break;
			}
			target << std::endl;
			target << std::endl;
		}

		virtual void visit(const hexabus::QueryPacket& query) {}
		virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) {}

		virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo)
		{
			if (endpointInfo.eid() == 0) {
				target << "Device Info" << std::endl
					<< "Device Name:\t" << endpointInfo.value() << std::endl;
			} else {
				target << "Endpoint Info\n"
					<< "Endpoint ID:\t" << endpointInfo.eid() << std::endl
					<< "EP Datatype:\t";
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
					case HXB_DTYPE_DATETIME:
						target << "Datetime";
						break;
					case HXB_DTYPE_FLOAT:
						target << "Float";
						break;
					case HXB_DTYPE_TIMESTAMP:
						target << "Timestamp";
						break;
					case HXB_DTYPE_128STRING:
						target << "String";
						break;
					case HXB_DTYPE_66BYTES:
						target << "Binary";
						break;
					default:
						target << "(unknown)";
						break;
				}
				target << std::endl;
				target << "EP Name:\t" << endpointInfo.value() << std::endl;
			}
			target << std::endl;
		}

		virtual void visit(const hexabus::InfoPacket<bool>& info) { printValuePacket(info, "Bool"); }
		virtual void visit(const hexabus::InfoPacket<uint8_t>& info) { printValuePacket(info, "UInt8"); }
		virtual void visit(const hexabus::InfoPacket<uint32_t>& info) { printValuePacket(info, "UInt32"); }
		virtual void visit(const hexabus::InfoPacket<float>& info) { printValuePacket(info, "Float"); }
		virtual void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info) { printValuePacket(info, "Datetime"); }
		virtual void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info) { printValuePacket(info, "Timestamp"); }
		virtual void visit(const hexabus::InfoPacket<std::string>& info) { printValuePacket(info, "String"); }
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (16 bytes)"); }
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (66 bytes)"); }

		virtual void visit(const hexabus::WritePacket<bool>& write) {}
		virtual void visit(const hexabus::WritePacket<uint8_t>& write) {}
		virtual void visit(const hexabus::WritePacket<uint32_t>& write) {}
		virtual void visit(const hexabus::WritePacket<float>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) {}
		virtual void visit(const hexabus::WritePacket<std::string>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
};

void print_packet(const hexabus::Packet& packet)
{
	PacketPrinter pp(std::cout);

	pp.visitPacket(packet);
}

datetime make_datetime_struct(time_t given_time = -1) {
    struct datetime value;
    time_t raw_time;
    tm *tm_time;

    if(given_time == -1) {
        time(&raw_time);
    } else {
        raw_time = given_time;
    }

    tm_time = localtime(&raw_time);

    value.hour = (uint8_t) tm_time->tm_hour;
    value.minute = (uint8_t) tm_time->tm_min;
    value.second = (uint8_t) tm_time->tm_sec;
    value.day = (uint8_t) tm_time->tm_mday;
    value.month = (uint8_t) tm_time->tm_mon + 1;
    value.year = (uint16_t) tm_time->tm_year + 1900;
    value.weekday = (uint8_t) tm_time->tm_wday;

    return value;

}

po::variable_value get_mandatory_parameter(
    po::variables_map vm,
    std::string param_id,
    std::string error_message
    )
{
  po::variable_value retval;
  try {
    if (! vm.count(param_id)) {
      std::cerr << error_message << std::endl;
      exit(-1);
    } else {
      retval=(vm[param_id]);
    }
  } catch (std::exception& e) {
    std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
    exit(-1);
  }
  return retval;
}


void send_packet(hexabus::Socket* net, const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet, bool printResponse = false)
{
  try {
    net->send(packet, addr);
  } catch (const hexabus::NetworkException& e) {
    std::cerr << "Could not send packet to " << addr << ": " << e.code().message() << std::endl;
    exit(1);
  }
  if (printResponse) {
		while (true) {
			std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> pair;
			try {
				pair = net->receive();
			} catch (const hexabus::GenericException& e) {
				const hexabus::NetworkException* nerror;
				if ((nerror = dynamic_cast<const hexabus::NetworkException*>(&e))) {
					std::cerr << "Error receiving packet: " << nerror->code().message() << std::endl;
				} else {
					std::cerr << "Error receiving packet: " << e.what() << std::endl;
				}
				exit(1);
			}

			if (pair.second.address() == addr) {
				print_packet(*pair.first);
				break;
			}
		}
  }
}

template<template<typename TValue> class ValuePacket>
void send_value_packet(hexabus::Socket* net, const boost::asio::ip::address_v6& ip, uint32_t eid, uint8_t datatype, const std::string& value)
{
	try { // handle errors in value lexical_cast
		switch (datatype) {
			case HXB_DTYPE_BOOL:
				{
					bool b = boost::lexical_cast<unsigned int>(value);
					std::cout << "Sending value " << b << std::endl;
					send_packet(net, ip, ValuePacket<bool>(eid, b));
				}
				break;
			case HXB_DTYPE_UINT8:
				{
					uint8_t u8 = boost::lexical_cast<unsigned int>(value);
					std::cout << "Sending value " << (unsigned int) u8 << std::endl;
					send_packet(net, ip, ValuePacket<uint8_t>(eid, u8));
				}
				break;
			case HXB_DTYPE_UINT32:
				{
					uint32_t u32 = boost::lexical_cast<uint32_t>(value);
					std::cout << "Sending value " << u32 << std::endl;
					send_packet(net, ip, ValuePacket<uint32_t>(eid, u32));
				}
				break;
			case HXB_DTYPE_FLOAT:
				{
					float f = boost::lexical_cast<float>(value);
					std::cout << "Sending value " << f << std::endl;
					send_packet(net, ip, ValuePacket<float>(eid, f));
				}
				break;
			case HXB_DTYPE_128STRING:
				{
					std::cout << "Sending value " << value << std::endl;
					send_packet(net, ip, ValuePacket<std::string>(eid, value));
				}
				break;
			default:
				{
					std::cout << "unknown data type " << datatype << std::endl;
				}
		}
	} catch (boost::bad_lexical_cast& e) {
		std::cerr << "Error while converting value: " << e.what() << std::endl;
		exit(1);
	}
}

int main(int argc, char** argv) {

  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " IP [additional options] ACTION";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexabus version and exit")
    ("command,c", po::value<std::string>(), "{get|set|epquery|send|listen|on|off|status|power|devinfo}")
    ("ip,i", po::value<std::string>(), "the hostname to connect to")
    ("bind,b", po::value<std::string>(), "local IP address to use")
    ("interface,I", po::value<std::string>(), "the interface to use for outgoing messages")
    ("eid,e", po::value<uint32_t>(), "Endpoint ID (EID)")
    ("datatype,d", po::value<unsigned int>(), "{1: Bool | 2: UInt8 | 3: UInt32 | 4: HexaTime | 5: Float | 6: String}")
    ("value,v", po::value<std::string>(), "Value")
    ("reliable,r", po::bool_switch(), "Reliable initialization of network access (adds delay, only needed for broadcasts)")
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
    exit(-1);
  }

  std::string command;
  boost::asio::ip::address_v6 bind_addr(boost::asio::ip::address_v6::any());

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  if (vm.count("version")) {
    std::cout << "hexaswitch -- command line hexabus client" << std::endl;
    std::cout << "libhexabus version " << hexabus::version() << std::endl;
    return 0;
  }

  if (! vm.count("command")) {
    std::cerr << "You must specify a command." << std::endl;
    return 1;
  } else {
    command=(vm["command"].as<std::string>());
  }

  if (vm.count("bind")) {
    bind_addr = boost::asio::ip::address_v6::from_string(vm["bind"].as<std::string>());
    std::cout << "Binding to " << bind_addr << std::endl;
  }

	boost::asio::io_service io;
  hexabus::Socket* network;

  if (vm.count("interface")) {
    std::string interface=(vm["interface"].as<std::string>());
    std::cout << "Using interface " << interface << std::endl;
    try {
      network=new hexabus::Socket(io, interface);
    } catch (const hexabus::NetworkException& e) {
      std::cerr << "Could not open socket on interface " << interface << ": " << e.code().message() << std::endl;
      return 1;
    }
  } else {
    try {
      network=new hexabus::Socket(io);
    } catch (const hexabus::NetworkException& e) {
      std::cerr << "Could not open socket: " << e.code().message() << std::endl;
      return 1;
    }
  }

  if(boost::iequals(command, std::string("LISTEN")))
  {
    std::cout << "Entering listen mode." << std::endl;

		network->listen(bind_addr);
		while (true) {
			std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> pair;
			try {
				pair = network->receive();
			} catch (const hexabus::GenericException& e) {
				const hexabus::NetworkException* nerror;
				if ((nerror = dynamic_cast<const hexabus::NetworkException*>(&e))) {
					std::cerr << "Error receiving packet: " << nerror->code().message() << std::endl;
				} else {
					std::cerr << "Error receiving packet: " << e.what() << std::endl;
				}
				exit(1);
			}

			std::cout << "Received packet from " << pair.second << std::endl;
			print_packet(*pair.first);
		}
  }

	network->bind(bind_addr);

	hexabus::LivenessReporter* liveness =
    vm.count("reliable") && vm["reliable"].as<bool>()
    ? new hexabus::LivenessReporter(*network)
    : 0;
	if (liveness) {
		liveness->establishPaths(1);
		liveness->start();
	}
  /*
   * Shorthand convenience commands.
   */

  if(boost::iequals(command, "ON") || boost::iequals(command, "OFF")) {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command needs an IP address").as<std::string>();
		send_packet(network, boost::asio::ip::address_v6::from_string(ip), hexabus::WritePacket<bool>(1, boost::iequals(command, "ON")));
  }
  else if(boost::iequals(command, std::string("STATUS"))) { // status: query EID 1
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command STATUS needs an IP address").as<std::string>();
		send_packet(network, boost::asio::ip::address_v6::from_string(ip), hexabus::QueryPacket(1), true);

  }
  else if(boost::iequals(command, std::string("POWER")))  // power: query EID 2
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command POWER needs an IP address").as<std::string>();
		send_packet(network, boost::asio::ip::address_v6::from_string(ip), hexabus::QueryPacket(2), true);
  }

  /*
   * Generic EID commands.
   */

  else if(boost::iequals(command, std::string("SET"))) // set: set an arbitrary EID
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command SET needs an IP address").as<std::string>();
    uint32_t eid = get_mandatory_parameter(vm,
        "eid", "command SET needs an EID and a datatype").as<uint32_t>();
    unsigned int dtype = get_mandatory_parameter(vm,
        "datatype", "command SET needs an EID and a datatype").as<unsigned int>();
		std::string value = get_mandatory_parameter(vm, "value", "command SET needs a value").as<std::string>();
		send_value_packet<hexabus::WritePacket>(network, boost::asio::ip::address_v6::from_string(ip), eid, dtype, value);
  }

  else if(boost::iequals(command, std::string("GET"))) // get: request the value of an arbitrary EID
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command GET needs an IP address").as<std::string>();
    uint32_t eid = get_mandatory_parameter(vm,
          "eid", "command GET needs an EID").as<uint32_t>();
		send_packet(network, boost::asio::ip::address_v6::from_string(ip), hexabus::QueryPacket(eid), true);
  }

  else if (boost::iequals(command, std::string("EPQUERY")))   // epquery: request endpoint metadata
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command EPQUERY needs an IP address").as<std::string>();
    uint32_t eid = get_mandatory_parameter(vm,
        "eid", "command EPQUERY needs an EID").as<uint32_t>();
		send_packet(network, boost::asio::ip::address_v6::from_string(ip), hexabus::EndpointQueryPacket(eid), true);
  }
  else if (boost::iequals(command, std::string("DEVINFO"))) 
    // epquery: request endpoint metadata
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command DEVINFO needs an IP address").as<std::string>();
		send_packet(network, boost::asio::ip::address_v6::from_string(ip), hexabus::EndpointQueryPacket(0), true);
  }
  else if (boost::iequals(command, std::string("SEND")))      // send: send a value broadcast
  {
    uint32_t eid = get_mandatory_parameter(vm,
        "eid", "command SEND needs an EID").as<uint32_t>();
    unsigned int dtype = get_mandatory_parameter(vm,
        "datatype", "command SEND needs a datatype").as<unsigned int>();
		std::string value = get_mandatory_parameter(vm, "value", "command SEND needs a value").as<std::string>();
		send_value_packet<hexabus::WritePacket>(network, hexabus::Socket::GroupAddress, eid, dtype, value);
  } else {
    std::cerr << "Unknown command." << std::endl;
    exit(1);
  }
  exit(0);
}

