#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/foreach.hpp>
#include <libhexabus/common.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <libhexabus/socket.hpp>

// STM_EQ ... used to define the enums in libhbc clash with constants defined in libhexabus
// TODO fix this by renaming the enums!
#undef STM_EQ
#undef STM_LEQ
#undef STM_GEQ
#undef STM_LT
#undef STM_GT
#undef STM_NEQ
#include <libhbc/ast_datatypes.hpp>
#include <libhbc/skipper.hpp>
#include <libhbc/hbcomp_grammar.hpp>
#include "../../../shared/endpoints.h"

namespace po = boost::program_options;

struct endpoint_descriptor 
{
	uint32_t eid;
	uint8_t datatype;
	std::string name;
};

struct device_descriptor
{
	boost::asio::ip::address_v6 ipv6_address;
	std::string name;
	std::vector<uint32_t> endpoint_ids;
};

struct ResponseHandler : public hexabus::PacketVisitor
{
	public:
		ResponseHandler(device_descriptor& device, std::vector<endpoint_descriptor>& endpoints) : _device(device), _endpoints(endpoints) {}

    // The uninteresing ones
		void visit(const hexabus::ErrorPacket& error) {}
		void visit(const hexabus::QueryPacket& query) {}
		void visit(const hexabus::EndpointQueryPacket& endpointQuery) {}

		void visit(const hexabus::InfoPacket<bool>& info) {}
		void visit(const hexabus::InfoPacket<uint8_t>& info) {}
		void visit(const hexabus::InfoPacket<float>& info) {}
		void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info) {}
		void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info) {}
		void visit(const hexabus::InfoPacket<std::string>& info) {}
		void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {}
		void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {}

		void visit(const hexabus::WritePacket<bool>& write) {}
		void visit(const hexabus::WritePacket<uint8_t>& write) {}
		void visit(const hexabus::WritePacket<uint32_t>& write) {}
		void visit(const hexabus::WritePacket<float>& write) {}
		void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) {}
		void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) {}
		void visit(const hexabus::WritePacket<std::string>& write) {}
		void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
		void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}

		void visit(const hexabus::EndpointInfoPacket& endpointInfo)
		{
			if(endpointInfo.eid() == EP_DEVICE_DESCRIPTOR) // If it's endpoint 0 (device descriptor), it contains the device name.
			{
				_device.name = endpointInfo.value();
			} else {
				endpoint_descriptor ep;
				ep.eid = endpointInfo.eid();
				ep.datatype = endpointInfo.datatype();
				ep.name = endpointInfo.value();

				_endpoints.push_back(ep);
			}
		}

		void visit(const hexabus::InfoPacket<uint32_t>& info)
		{
			if(info.eid() == 0) // we only care for the device descriptor
			{
				uint32_t val = info.value();
				for(int i = 0; i < 32; ++i)
				{
					if(val % 2) // find out whether LSB is set
						_device.endpoint_ids.push_back(i); // if it's set, store the EID (the bit's position in the device descriptor).

					val >>= 1; // right-shift in order to have the next EID in the LSB
				}
			}
		}

	private:
		device_descriptor& _device;
		std::vector<endpoint_descriptor>& _endpoints;
};

void print_dev_info(const device_descriptor& dev)
{
	std::cout << "Device information:" << std::endl;
	std::cout << "\tIP address: \t" << dev.ipv6_address.to_string() << std::endl;
	std::cout << "\tDevice name: \t" << dev.name << std::endl;
	std::cout << "\tEndpoints: \t";
	for(std::vector<uint32_t>::const_iterator it = dev.endpoint_ids.begin(); it != dev.endpoint_ids.end(); ++it)
		std::cout << *it << " ";
	std::cout << std::endl;
}

void print_ep_info(const endpoint_descriptor& ep)
{
	std::cout << "Endpoint Information:" << std::endl;
	std::cout << "\tEndpoint ID:\t" << ep.eid << std::endl;
	std::cout << "\tData type:\t";
	switch(ep.datatype)
	{
		case HXB_DTYPE_BOOL:
			std::cout << "Bool"; break;
		case HXB_DTYPE_UINT8:
			std::cout << "UInt8"; break;
		case HXB_DTYPE_UINT32:
			std::cout << "UInt32"; break;
		case HXB_DTYPE_DATETIME:
			std::cout << "Datetime"; break;
		case HXB_DTYPE_FLOAT:
			std::cout << "Float"; break;
		case HXB_DTYPE_TIMESTAMP:
			std::cout << "Timestamp"; break;
		case HXB_DTYPE_128STRING:
			std::cout << "String"; break;
		case HXB_DTYPE_16BYTES:
			std::cout << "Binary (16bytes)"; break;
		case HXB_DTYPE_66BYTES:
			std::cout << "Binary (66bytes)"; break;
		default:
			std::cout << "(unknown)"; break;
	}
	std::cout << std::endl;
	std::cout << "\tName:\t\t" << ep.name <<std::endl;
	std::cout << std::endl;
}

std::string remove_specialchars(std::string s)
{
	size_t space;
	while( // only leave in the characters that Hexabus Compiler can handle.
		(space = s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"))
		!= std::string::npos
		)
		s = s.erase(space, 1);

	// the first character can't be a number. If it is, delete it.
	while(std::string("0123456789").find(s[0]) != std::string::npos)
		s = s.substr(1);

	// device/endpoint name can't be empty. Let's make it "_empty_" instead.
	if(s.length() == 0)
		s = "_empty_";

	return s;
}

void write_dev_desc(const device_descriptor& dev, std::ostream& target)
{
	target << "device " << remove_specialchars(dev.name) << " {" << std::endl;
	target << "\tip " << dev.ipv6_address.to_string() << ";" << std::endl;
	target << "\teids { ";
	for(std::vector<uint32_t>::const_iterator it = dev.endpoint_ids.begin(); it != dev.endpoint_ids.end(); ) // no increment here!
	{
		target << *it;
		if(++it != dev.endpoint_ids.end()) // increment here to see if we have to put a comma
			target << ", ";
	}
	target << " };" << std::endl;
	target << "}" << std::endl;
}

void write_ep_desc(const endpoint_descriptor& ep, std::ostream& target)
{
	if(ep.datatype == HXB_DTYPE_BOOL     // only write output for datatypes the Hexabs Compiler can handle
		|| ep.datatype == HXB_DTYPE_UINT8
		|| ep.datatype == HXB_DTYPE_UINT32
		|| ep.datatype == HXB_DTYPE_FLOAT)
	{
		target << "# Auto-generated by hexinfo." << std::endl;
		target << "endpoint " << remove_specialchars(ep.name) << " {" << std::endl;
		target << "\teid " << ep.eid << ";" << std::endl;
		target << "\tdatatype ";
		switch(ep.datatype)
		{
			case HXB_DTYPE_BOOL:
				target << "BOOL"; break;
			case HXB_DTYPE_UINT8:
				target << "UINT8"; break;
			case HXB_DTYPE_UINT32:
				target << "UINT32"; break;
			case HXB_DTYPE_FLOAT:
				target << "FLOAT"; break;
			default:
				target << "(unknown)"; break;
		}
		target << ";" << std::endl;
		target << "\taccess { broadcast, write } #TODO auto-generated template. Please edit!" << std::endl;
		target << "}" << std::endl << std::endl;
	}
}

void send_packet(hexabus::Socket* net, const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet, ResponseHandler& handler)
{
	try {
		net->send(packet, addr);
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Could not send packet to " << addr << ": " << e.code().message() << std::endl;
		exit(1);
	}

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
			handler.visitPacket(*pair.first);
			break;
		}
	}
}

hexabus::hbc_doc read_file(std::string filename, bool verbose)
{
	// read the file, and parse the endpoint definitions
	bool r = false;
	std::ifstream in(filename.c_str(), std::ios_base::in);
	if(verbose)
		std::cout << "Reading input file " << filename << "..." << std::endl;

	if(!in) {
		std::cerr << "Error: Could not open input file: " << filename << std::endl;
		exit(1);
	}

	in.unsetf(std::ios::skipws); // no white space skipping, this will be handled by the parser

	typedef std::istreambuf_iterator<char> base_iterator_type;
	typedef boost::spirit::multi_pass<base_iterator_type> forward_iterator_type;
	typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;

	base_iterator_type in_begin(in);
	forward_iterator_type fwd_begin = boost::spirit::make_default_multi_pass(in_begin);
	forward_iterator_type fwd_end;
	pos_iterator_type position_begin(fwd_begin, fwd_end, filename);
	pos_iterator_type position_end;

	std::vector<std::string> error_hints;
	typedef hexabus::hexabus_comp_grammar<pos_iterator_type> hexabus_comp_grammar;
	typedef hexabus::skipper<pos_iterator_type> Skip;
	hexabus_comp_grammar grammar(error_hints, position_begin);
	Skip skipper;
	hexabus::hbc_doc ast; // The AST

	using boost::spirit::ascii::space;
	using boost::spirit::ascii::char_;
	try {
		r = phrase_parse(position_begin, position_end, grammar, skipper, ast);
	} catch(const qi::expectation_failure<pos_iterator_type>& e) {
		const classic::file_position_base<std::string>& pos = e.first.get_position();
		std::cerr << "Error in " << pos.file << " line " << pos.line << " column " << pos.column << std::endl
			<< "'" << e.first.get_currentline() << "'" << std::endl
			<< std::setw(pos.column) << " " << "^-- " << *hexabus::error_traceback_t::stack.begin() << std::endl;
	} catch(const std::runtime_error& e) {
		std::cout << "Exception occured: " << e.what() << std::endl;
	}

	if(r && position_begin == position_end) {
		if(verbose)
			std::cout << "Parsing of file " << filename << " succeeded." << std::endl;
	} else {
		std::cerr << "Parsing of file " << filename << " failed." << std::endl;
		exit(1);
	}

	return ast;
}

int main(int argc, char** argv)
{
	// the command line interface
	std::ostringstream oss;
	oss << "Usage: " << argv[0] << " IP [additional options]";
	po::options_description desc(oss.str());
	desc.add_options()
		("help,h", "produce help message")
		("version", "print version and exit")
		("ip,i", po::value<std::string>(), "IP addres of device")
		("print,p", "print device and endpoint info to the console")
		("epfile,e", po::value<std::string>(), "name of Hexabus Compiler header file to write the endpoint list to")
		("devfile,d", po::value<std::string>(), "name of Hexabus Compiler header file to write the device definition to")
		("verbose,V", "print more status information")
		;

	po::positional_options_description p;
	p.add("ip", 1);

	po::variables_map vm;

	// begin processing of command line parameters
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
		po::notify(vm);
	} catch (std::exception& e) {
		std::cerr << "Cannot process command line: " << e.what() << std::endl;
		exit(-1);
	}

	if(vm.count("help"))
	{
		std::cout << desc << std::endl;
		return 1;
	}

	if(vm.count("version"))
	{
		std::cout << "hexinfo -- hexabus endpoint enumeration tool" << std::endl;
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		return 0;
	}

	bool verbose = false;
	if(vm.count("verbose"))
		verbose = true;

	if(!vm.count("ip"))
	{
		std::cerr << "You must specify an IP address." << std::endl;
		return 1;
	} else { // we have exactly one IP address
		// init the network interface
		boost::asio::io_service io;
		hexabus::Socket* network;
		try {
			network = new hexabus::Socket(io);
		} catch(const hexabus::NetworkException& e) {
			std::cerr << "Could not open socket: " << e.code().message() << std::endl;
			return 1;
		}
		boost::asio::ip::address_v6 bind_addr(boost::asio::ip::address_v6::any());
		network->bind(bind_addr);

		boost::asio::ip::address_v6 target_ip = boost::asio::ip::address_v6::from_string(vm["ip"].as<std::string>());

		// construct the responseHandler
		device_descriptor device;
		device.ipv6_address = target_ip; // we can already set the IP address
		std::vector<endpoint_descriptor> endpoints;
		ResponseHandler handler(device, endpoints);

		// send the device name query packet and listen for the reply
		send_packet(network, target_ip, hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR), handler);

		// send the device descriptor (endpoint list) query packet and listen for the reply
		send_packet(network, target_ip, hexabus::QueryPacket(EP_DEVICE_DESCRIPTOR), handler);

		// now, iterate over the endpoint list and find out the properties of each endpoint
		for(std::vector<uint32_t>::iterator it = device.endpoint_ids.begin(); it != device.endpoint_ids.end(); ++it)
			send_packet(network, target_ip, hexabus::EndpointQueryPacket(*it), handler);

		if(vm.count("print"))
		{
			// print the information onto the command line
			print_dev_info(device);
			std::cout << std::endl;
			for(std::vector<endpoint_descriptor>::iterator it = endpoints.begin(); it != endpoints.end(); ++it)
				print_ep_info(*it);
		}

		if(vm.count("epfile"))
		{
			// read in HBC file
			hexabus::hbc_doc hbc_input = read_file(vm["epfile"].as<std::string>(), verbose);

			std::set<uint32_t> existing_eids;
			// check all endpoint definitions, insert eids into set of existing eids
			BOOST_FOREACH(hexabus::hbc_block block, hbc_input.blocks)
			{
				if(block.which() == 1) // endpoint_doc
				{
					BOOST_FOREACH(hexabus::endpoint_cmd_doc epc, boost::get<hexabus::endpoint_doc>(block).cmds)
					{
						if(epc.which() == 0) // ep_eid_doc
						{
							existing_eids.insert(boost::get<hexabus::ep_eid_doc>(epc).eid);
						}
					}
				}
			}

			if(verbose)
			{
				std::cout << "Endpoint IDs already present in file: ";
				for(std::set<uint32_t>::iterator it = existing_eids.begin(); it != existing_eids.end(); ++it)
				{
					std::cout << *it << " ";
				}
				std::cout << std::endl;
			}

			// Write (newly discovered) eids to the file
			std::ofstream ofs;
			ofs.open(vm["epfile"].as<std::string>().c_str(), std::fstream::app); // TODO do it without opening the file twice
			if(!ofs)
			{
				std::cerr << "Error: Could not open output file: " << vm["devfile"].as<std::string>().c_str() << std::endl;
				return 1;
			}

			for(std::vector<endpoint_descriptor>::iterator it = endpoints.begin(); it != endpoints.end(); ++it)
			{
				if(!existing_eids.count(it->eid))
					write_ep_desc(*it, ofs);
			}
		}

		// TODO devfile
	}
}
