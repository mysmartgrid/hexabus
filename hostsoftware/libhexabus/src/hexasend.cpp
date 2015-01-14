#include <iostream>
#include <libhexabus/socket.hpp>
#include <libhexabus/error.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>

#include "shared.hpp"

#include "resolv.hpp"

using namespace hexabus;
namespace po = boost::program_options;

namespace {

void sendPacket(Socket& net, const boost::asio::ip::address_v6& addr, const Packet& packet)
{
	try {
		net.send(packet, addr);
	} catch (const NetworkException& e) {
		std::cerr << "could not send packet to " << addr << ": " << e.code().message() << std::endl;
		std::exit(1);
	}
}

template<size_t Len>
boost::array<char, Len> parseBinary(std::string value)
{
	if (value.size() < 3 ||
			value.size() > 2 + 2 * Len ||
			value[0] != '0' ||
			(value[1] != 'x' && value[1] != 'X'))
		throw std::bad_cast();

	value = value.substr(2);
	if (value.size() % 2)
		value = "0" + value;

	boost::array<char, Len> result;

	result.assign(0);

	auto target = result.begin() + (Len - value.size() / 2);
	for (unsigned pos = 0; pos < value.size(); pos += 2, ++target) {
		if (!std::isxdigit(value[pos]) || !std::isxdigit(value[pos + 1]))
			throw std::bad_cast();
		*target = std::stoul(value.substr(pos, 2), nullptr, 16);
	}

	return result;
}

template<template<typename TValue> class PClass>
std::unique_ptr<Packet> parsePacket(hxb_datatype dt, uint32_t eid, const std::string& value)
{
	typedef std::unique_ptr<Packet> pptr;

	try {
		switch (dt) {
		case HXB_DTYPE_BOOL: return pptr(new PClass<bool>(eid, boost::lexical_cast<bool>(value)));
		case HXB_DTYPE_UINT16: return pptr(new PClass<uint16_t>(eid, boost::lexical_cast<uint16_t>(value)));
		case HXB_DTYPE_UINT32: return pptr(new PClass<uint32_t>(eid, boost::lexical_cast<uint32_t>(value)));
		case HXB_DTYPE_UINT64: return pptr(new PClass<uint64_t>(eid, boost::lexical_cast<uint64_t>(value)));
		case HXB_DTYPE_SINT16: return pptr(new PClass<int16_t>(eid, boost::lexical_cast<int16_t>(value)));
		case HXB_DTYPE_SINT32: return pptr(new PClass<int32_t>(eid, boost::lexical_cast<int32_t>(value)));
		case HXB_DTYPE_SINT64: return pptr(new PClass<int64_t>(eid, boost::lexical_cast<int64_t>(value)));
		case HXB_DTYPE_FLOAT: return pptr(new PClass<float>(eid, boost::lexical_cast<float>(value)));
		case HXB_DTYPE_128STRING: return pptr(new PClass<std::string>(eid, value));
		case HXB_DTYPE_65BYTES: return pptr(new PClass<boost::array<char, 65>>(eid, parseBinary<65>(value)));
		case HXB_DTYPE_16BYTES: return pptr(new PClass<boost::array<char, 16>>(eid, parseBinary<16>(value)));

		case HXB_DTYPE_UINT8: {
			auto parsed = boost::lexical_cast<unsigned>(value);
			if (parsed > 255)
				throw boost::bad_lexical_cast();
			return pptr(new PClass<uint8_t>(eid, parsed));
		}
		case HXB_DTYPE_SINT8: {
			auto parsed = boost::lexical_cast<int>(value);
			if (parsed < -128 || parsed > 127)
				throw boost::bad_lexical_cast();
			return pptr(new PClass<int8_t>(eid, parsed));
		}

		case HXB_DTYPE_UNDEFINED:
			break;
		}
	} catch (const std::exception& e) {
		std::cerr << "invalid value " << value << std::endl;
		std::exit(1);
	}

	std::cerr << "invalid datatype" << std::endl;
	std::exit(1);
}

}

int main(int argc, char* argv[])
{
	po::options_description desc;
	desc.add_options()
		("help,h", "display this message")
		("command,c", po::value<std::string>(), "packet type to send (query|write|epquery|info)")
		("ip,i", po::value<std::string>(), "address to send to")
		("bind,b", po::value<std::string>(), "address to send from")
		("interface,I", po::value<std::string>(), "interface to send from (only for info packets)")
		("eid,e", po::value<uint32_t>())
		("datatype,d", po::value<std::string>(), "data type for packet\n"
			"  \tvalid types:\n"
			"    \tBool, (U)Int8, (U)Int16, (U)Int32, (U)Int64, Float, String, Binary(16), Binary(65)\n"
			"  \tshort forms:\n"
			"    \tb, u8, s8, u16, s16, u32, s32, u64, s64, f, s, b16, b65")
		("value,v", po::value<std::string>(), "value to send")
		("oneline,o", po::bool_switch(), "display response in single line")
		("timeout,t", po::value<uint32_t>()->default_value(1000), "timeout (in ms) to wait for response, 0 waits forever");

	po::positional_options_description positional;
	positional.add("command", 1);
	positional.add("ip", 1);

	po::variables_map vm;

	try {
		po::store(
			po::command_line_parser(argc, argv)
				.options(desc)
				.positional(positional)
				.run(),
			vm);
		po::notify(vm);
	} catch (const std::exception& e) {
		std::cerr << "error in command line: unexpected option " << e.what() << std::endl;
		return 1;
	}

	if (vm.count("help") || argc == 1) {
		std::cout << "Usage: hexasend <command> <ip> [additional options]" << std::endl;
		std::cout << desc;
		return 0;
	}

	if (!vm.count("command")) {
		std::cerr << "command required" << std::endl;
		return 1;
	}

	boost::asio::io_service io;
	Socket socket(io);

	auto command = vm["command"].as<std::string>();

	if (vm.count("bind")) {
		boost::system::error_code err;
		auto bindStr = vm["bind"].as<std::string>();

		auto addr = resolve(io, bindStr, err);
		if (err) {
			std::cerr << "cannot bind to " << bindStr << ": " << err.message() << std::endl;
			return 1;
		}

		try {
			socket.bind(addr);
		} catch (const NetworkException& e) {
			std::cerr << "cannot bind to " << bindStr << ": " << e.code().message() << std::endl;
			return 1;
		}
	}

	if (vm.count("interface")) {
		auto iface = vm["interface"].as<std::string>();

		try {
			socket.mcast_from(iface);
		} catch (const NetworkException& e) {
			std::cerr << "cannot bind to " << iface << ": " << e.code().message() << std::endl;
			return 1;
		}
	}

	auto require = [&] (const std::string& param) {
		if (!vm.count(param)) {
			std::cerr << "required parameter --" << param << " is missing" << std::endl;
			std::exit(1);
		}
	};
	auto ip = [&] () {
		require("ip");
		boost::system::error_code err;

		auto addr = resolve(io, vm["ip"].as<std::string>(), err);
		if (err) {
			std::cerr << "cannot send to " << vm["ip"].as<std::string>() << ": " << err.message() << std::endl;
			std::exit(1);
		}

		return addr;
	};
	auto eid = [&] () {
		require("eid");
		return vm["eid"].as<uint32_t>();
	};
	auto datatype = [&] () {
		require("datatype");
		auto result = dtypeStrToDType(vm["datatype"].as<std::string>());
		if (result < 0) {
			std::cerr << "invalid datatype " << vm["datatype"].as<std::string>() << std::endl;
			std::exit(1);
		}
		return (hxb_datatype) result;
	};
	auto value = [&] () {
		require("value");
		return vm["value"].as<std::string>();
	};

	auto receiveReply = [&] () {
		boost::posix_time::time_duration timeout = boost::posix_time::milliseconds(vm["timeout"].as<uint32_t>());
		if (!timeout.total_milliseconds())
			timeout = boost::date_time::pos_infin;

		auto response = socket.receive(hexabus::filtering::any(), timeout);

		if (response.first)
			PacketPrinter(vm.count("oneline") && vm["oneline"].as<bool>())(*response.first, response.second);
		else
			std::cout << "timeout" << std::endl;
	};

	if (command == "query") {
		sendPacket(socket, ip(), QueryPacket(eid()));
		receiveReply();
	} else if (command == "info" || command == "write") {
		auto packet = command == "info"
			? parsePacket<InfoPacket>(datatype(), eid(), value())
			: parsePacket<WritePacket>(datatype(), eid(), value());

		auto addr = Socket::GroupAddress;
		if (vm.count("ip"))
			addr = ip();

		if (addr.is_multicast())
			require("interface");

		try {
			socket.send(*packet, addr);
		} catch (const NetworkException& e) {
			std::cerr << "failed to send packet: " << e.code().message() << std::endl;
			return 1;
		}
	} else if (command == "epquery") {
		sendPacket(socket, ip(), EndpointQueryPacket(eid()));
		receiveReply();
	} else {
		std::cerr << "unknown command " << command << std::endl;
		return 1;
	}

	return 0;
}
