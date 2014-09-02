#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/foreach.hpp>
#include <libhexabus/common.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/endpoint_registry.hpp>

#include <libhbc/ast_datatypes.hpp>
#include <libhbc/skipper.hpp>
#include <libhbc/hbcomp_grammar.hpp>
#include "../../../shared/endpoints.h"
#include "../../../shared/hexabus_definitions.h"

namespace po = boost::program_options;

struct property_descriptor
{
	uint32_t propid;
	uint8_t datatype;
	boost::variant<
		bool,
		uint8_t,
		uint32_t,
		float,
		std::string
	> value;

	bool operator<(const property_descriptor &b) const
	{
		return (propid < b.propid);
	}
};

struct endpoint_descriptor
{
	uint32_t eid;
	uint8_t datatype;
	std::string name;
	std::set<property_descriptor> properties;
	bool operator<(const endpoint_descriptor &b) const
	{
		return (eid < b.eid);
	}
};

struct device_descriptor
{
	boost::asio::ip::address_v6 ipv6_address;
	std::string name;
	std::set<endpoint_descriptor> endpoint_ids;

	bool operator<(const device_descriptor &b) const
	{
		return (ipv6_address < b.ipv6_address);
	}
};

struct ReceiveCallback { // callback for device discovery
	std::set<boost::asio::ip::address_v6>* addresses;
	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep)
	{
		addresses->insert(asio_ep.address().to_v6());
	}
};

struct ReportCallback { // callback for populating data structures
	endpoint_descriptor* current_ep;
	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep)
	{
		const hexabus::EndpointReportPacket* endpointReport = dynamic_cast<const hexabus::EndpointReportPacket*>(&packet);
		if(endpointReport != NULL)
		{
			current_ep->eid = endpointReport->eid();
			current_ep->datatype = endpointReport->datatype();
			current_ep->name = endpointReport->value();
		}
	}
};

struct NameCallback {
	device_descriptor* dev;
	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep)
	{
		const hexabus::ReportPacket<std::string>* report = dynamic_cast<const hexabus::ReportPacket<std::string>*>(&packet);
		if(report != NULL)
		{
			dev->name = report->value();
		}
	}
};

struct PropreportCallback {
	endpoint_descriptor* current_ep;
	uint32_t current_pid;
	uint32_t next_eid;

	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep)
	{
		property_descriptor pd;

		do {
			const hexabus::PropertyReportPacket<bool>* propertyReport_b = dynamic_cast<const hexabus::PropertyReportPacket<bool>* >(&packet);
			if(propertyReport_b !=NULL) {
				pd.propid = current_pid;
				pd.datatype = HXB_DTYPE_BOOL;
				pd.value = propertyReport_b->value();
				current_pid = propertyReport_b->nextid();
				current_ep->properties.insert(pd);
				break;
			}
			const hexabus::PropertyReportPacket<uint8_t>* propertyReport_u8 = dynamic_cast<const hexabus::PropertyReportPacket<uint8_t>* >(&packet);
			if(propertyReport_u8 !=NULL) {
				pd.propid = current_pid;
				pd.datatype = HXB_DTYPE_UINT8;
				pd.value = propertyReport_u8->value();
				current_pid = propertyReport_u8->nextid();
				current_ep->properties.insert(pd);
				break;
			}
			const hexabus::PropertyReportPacket<uint32_t>* propertyReport_u32 = dynamic_cast<const hexabus::PropertyReportPacket<uint32_t>* >(&packet);
			if(propertyReport_u32 !=NULL) {
				pd.propid = current_pid;
				if(current_pid == 0) {
					next_eid = propertyReport_u32->value();
				}
				pd.datatype = HXB_DTYPE_UINT32;
				pd.value = propertyReport_u32->value();
				current_pid = propertyReport_u32->nextid();
				current_ep->properties.insert(pd);
				break;
			}
			const hexabus::PropertyReportPacket<float>* propertyReport_f = dynamic_cast<const hexabus::PropertyReportPacket<float>* >(&packet);
			if(propertyReport_f !=NULL) {
				pd.propid = current_pid;
				pd.datatype = HXB_DTYPE_FLOAT;
				pd.value = propertyReport_f->value();
				current_pid = propertyReport_f->nextid();
				current_ep->properties.insert(pd);
				break;
			}
			const hexabus::PropertyReportPacket<std::string>* propertyReport_s = dynamic_cast<const hexabus::PropertyReportPacket<std::string>* >(&packet);
			if(propertyReport_s !=NULL) {
				pd.propid = current_pid;
				pd.datatype = HXB_DTYPE_128STRING;
				pd.value = propertyReport_s->value();
				current_pid = propertyReport_s->nextid();
				current_ep->properties.insert(pd);
				break;
			}
			const hexabus::PropertyReportPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >* propertyReport_16b = dynamic_cast<const hexabus::PropertyReportPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >* >(&packet);
			if(propertyReport_16b !=NULL) {
				pd.propid = current_pid;
				pd.datatype = HXB_DTYPE_16BYTES;
				pd.value = std::string(propertyReport_16b->value().begin(), propertyReport_16b->value().end());
				current_pid = propertyReport_16b->nextid();
				current_ep->properties.insert(pd);
				break;
			}
			const hexabus::PropertyReportPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >* propertyReport_66b = dynamic_cast<const hexabus::PropertyReportPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >* >(&packet);
			if(propertyReport_66b !=NULL) {
				pd.propid = current_pid;
				pd.datatype = HXB_DTYPE_66BYTES;
				pd.value = std::string(propertyReport_66b->value().begin(), propertyReport_66b->value().end());
				current_pid = propertyReport_66b->nextid();
				current_ep->properties.insert(pd);
				break;
			}
		} while(false);
	}
};

struct ErrorCallback {
	void operator()(const hexabus::GenericException& error)
	{
		std::cerr << "Error receiving packet: " << error.what() << std::endl;
		exit(1);
	}
};

struct TransmissionCallback {
	boost::asio::io_service* io;

	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep, bool failed)
	{
		io->stop();
		if(failed) {
			std::cerr << "Error transmitting packet: Timeout" << std::endl;
			exit(1);
		}
	}
};

void print_dev_info(const device_descriptor& dev)
{
	std::cout << "Device information:" << std::endl;
	std::cout << "\tIP address: \t" << dev.ipv6_address.to_string() << std::endl;
	std::cout << "\tDevice type: \t" << dev.name << std::endl;
	std::cout << "\tEndpoints: \t";
	for(std::set<endpoint_descriptor>::const_iterator it = dev.endpoint_ids.begin(); it != dev.endpoint_ids.end(); ++it)
		std::cout << it->eid << " ";
	std::cout << std::endl;
}

void print_ep_info(const endpoint_descriptor& ep)
{
	std::cout << "\tEndpoint Information:" << std::endl;
	std::cout << "\t\tEndpoint ID:\t" << ep.eid << std::endl;
	std::cout << "\t\tData type:\t";
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
	std::cout << "\t\tName:\t\t" << ep.name <<std::endl;
	std::cout << std::endl;
}

void print_prop_info(const property_descriptor& pd)
{
	std::cout << "\t\tProperty Information:" << std::endl;
	std::cout << "\t\t\tProperty ID:\t" << pd.propid << std::endl;
	std::cout << "\t\t\tData type:\t";
	switch(pd.datatype)
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
	std::cout << "\t\t\tValue:\t" << pd.value << std::endl;
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
	target << "# Auto-generated by hexinfo." << std::endl;
	target << "device " << remove_specialchars(dev.name) << " {" << std::endl;
	target << "\tip " << dev.ipv6_address.to_string() << ";" << std::endl;
	target << "\teids { ";
	for(std::set<endpoint_descriptor>::const_iterator it = dev.endpoint_ids.begin(); it != dev.endpoint_ids.end(); ) // no increment here!
	{
		target << it->eid;
		if(++it != dev.endpoint_ids.end()) // increment here to see if we have to put a comma
			target << ", ";
	}
	target << " }" << std::endl;
	target << "}" << std::endl << std::endl;
}

struct json_string_writer {
	std::string str;
	json_string_writer(const std::string& str) : str(str) {}

	friend std::ostream& operator<<(std::ostream& target, const json_string_writer& jsw)
	{
		for (std::string::const_iterator it = jsw.str.begin(), end = jsw.str.end(); it != end; it++) {
			switch (*it) {
				case '"': target << "\""; break;
				case '\n': target << "\n"; break;
				case '\r': target << "\r"; break;
				case '\t': target << "\t"; break;
				case '\v': target << "\v"; break;
				default: target << *it;
			}
		}
		return target;
	}
};

json_string_writer json(const std::string& str)
{
	return json_string_writer(str);
}

bool write_dev_desc_json(const device_descriptor& dev, std::ostream& target, bool emit_comma, bool write_metadata)
{
	static hexabus::EndpointRegistry ep_registry;

	// if name == "", we might as well assume temporary communication problems
	if (dev.name.length() != 0) {
		if (emit_comma) {
			target << "," << std::endl;
		}
		target
			<< "{" << std::endl
			<< "\t\"name\": \"" << json(dev.name) << "\"," << std::endl
			<< "\t\"ip\": \"" << dev.ipv6_address << "\"," << std::endl
			<< "\t\"endpoints\": [" << std::endl;
		for (std::set<endpoint_descriptor>::const_iterator it = dev.endpoint_ids.begin(), end = dev.endpoint_ids.end(); it != end; ) {
			target
				<< "\t\t{" << std::endl
				<< "\t\t\"eid\": " << it->eid;

			hexabus::EndpointRegistry::const_iterator ep_it = ep_registry.find(it->eid);
			if (ep_it != ep_registry.end()) {
				target
					<< "," << std::endl
					<< "\t\t\"unit\": \"" << json(ep_it->second.unit().get_value_or("")) << "\"," << std::endl
					<< "\t\t\"description\": \"" << json(ep_it->second.description()) << "\"," << std::endl
					<< "\t\t\"function\": \"";
				switch (ep_it->second.function()) {
					case hexabus::EndpointDescriptor::sensor:         target << "sensor"; break;
					case hexabus::EndpointDescriptor::actor:          target << "actor"; break;
					case hexabus::EndpointDescriptor::infrastructure: target << "infrastructure"; break;
				}
				target << "\"," << std::endl
					<< "\t\t\"type\": " << ep_it->second.type();

				if(write_metadata) {
					target
						<< "," << std::endl
						<<  "\t\t\"properties\": [" << std::endl;

					for(std::set<property_descriptor>::const_iterator prop_it = it->properties.begin(), prop_end = it->properties.end(); prop_it != prop_end; ) {
						if(prop_it->propid != 0) {
							target
								<< "\t\t\t{" << std::endl
								<< "\t\t\t\"property ID\": " << prop_it->propid << "," << std::endl;

								if(boost::get<std::string>(&prop_it->value))
									target << "\t\t\t\"value\": \"" << prop_it->value << "\"" << std::endl;
								else
									target << "\t\t\t\"value\": " << prop_it->value << std::endl;

								target << "\t\t\t}";

							prop_it++;

							if(prop_it != prop_end) {
								target << ",";
							}
							target << std::endl;
						} else {
							prop_it++;
						}
					}
					target << "\t\t]" << std::endl;
				} else {
					target << std::endl;
				}
			} else {
				target << std::endl;
			}

			target << "\t\t}";

			it++;
			if (it != end) {
				target << ",";
			}
			target << std::endl;
		}
		target
			<< "\t]" << std::endl
			<< "}" << std::endl;

		return true;
	} else {
		return false;
	}
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

void send_packet(hexabus::Socket* net, const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet)
{
	try {
		net->send(packet, addr);
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Could not send packet to " << addr << ": " << e.code().message() << std::endl;
		exit(1);
	}
}

hexabus::hbc_doc read_file(std::string filename, bool verbose)
{
	hexabus::hbc_doc ast; // The AST

	// read the file, and parse the endpoint definitions
	bool r = false;
	std::ifstream in(filename.c_str(), std::ios_base::in);
	if(verbose)
		std::cout << "Reading input file " << filename << "..." << std::endl;

	if(!in) {
		std::cerr << "Could not open input file: " << filename << " -- does it exist? (I will try to create it for you!)" << std::endl;
		return ast;
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
		("ip,i", po::value<std::string>(), "IP address of device")
		("interface,I", po::value<std::string>(), "Interface to send multicast from")
		("discover,c", "automatically discover hexabus devices")
		("metadata,m", "aditionally query endpoint metadata")
		("print,p", "print device and endpoint info to the console")
		("epfile,e", po::value<std::string>(), "name of Hexabus Compiler header file to write the endpoint list to")
		("devfile,d", po::value<std::string>(), "name of Hexabus Compiler header file to write the device definition to")
		("json,j", "use JSON as output format")
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

	// set up the things we need later =========
	std::set<boost::asio::ip::address_v6> addresses; // Holds the list of addresses to scan -- either filled with everything we "discover" or with just one "ip"
	// init the network interface
	boost::asio::io_service io;
	hexabus::Socket* network = new hexabus::Socket(io, 5);
	try {
		if(vm.count("interface")) {
			if(verbose)
				std::cout << "Using network interface " << vm["interface"].as<std::string>() << "." << std::endl;
			network->mcast_from(vm["interface"].as<std::string>());
		}
	} catch(const hexabus::NetworkException& e) {
		std::cerr << "Could not open socket: " << e.code().message() << std::endl;
		return 1;
	}
	boost::asio::ip::address_v6 bind_addr(boost::asio::ip::address_v6::any());
	network->bind(bind_addr);

	// sets for storing the received data
	std::set<device_descriptor> devices;
	// =========================================


	if(vm.count("ip") && vm.count("discover"))
	{
		std::cerr << "Error: Options --ip and --discover are mutually exclusive." << std::endl;
		exit(1);
	}

	if(vm.count("discover"))
	{
		if(verbose)
			std::cout << "Discovering devices..." << std::endl;
		// send the packet
		boost::asio::ip::address_v6 hxb_broadcast_address = boost::asio::ip::address_v6::from_string(HXB_GROUP);
		send_packet(network, hxb_broadcast_address, hexabus::QueryPacket(EP_DEVICE_DESCRIPTOR));

		// call back handlers for receiving packets
		ReceiveCallback receiveCallback = { &addresses };
		ErrorCallback errorCallback;

		// use connections so that callback handlers get deleted (.disconnect())
		boost::signals2::connection c1 = network->onAsyncError(errorCallback);
		boost::signals2::connection c2 = network->onPacketReceived(receiveCallback, hexabus::filtering::isReport<std::string>() && (hexabus::filtering::eid() == EP_DEVICE_DESCRIPTOR));

		// timer that cancels receiving after n seconds
		boost::asio::deadline_timer timer(network->ioService());
		timer.expires_from_now(boost::posix_time::seconds(3)); // TODO do we want this configurable? Or at least as a constant
		timer.async_wait(boost::bind(&boost::asio::io_service::stop, &io));

		boost::asio::deadline_timer resend_timer1(network->ioService()); // resend packet twice, with 1 sec timout in between
		boost::asio::deadline_timer resend_timer2(network->ioService());
		resend_timer1.expires_from_now(boost::posix_time::seconds(1));
		resend_timer2.expires_from_now(boost::posix_time::seconds(2));
		resend_timer1.async_wait(boost::bind(&send_packet, network, hxb_broadcast_address, hexabus::QueryPacket(EP_DEVICE_DESCRIPTOR)));
		resend_timer2.async_wait(boost::bind(&send_packet, network, hxb_broadcast_address, hexabus::QueryPacket(EP_DEVICE_DESCRIPTOR)));

		network->ioService().run();

		c1.disconnect();
		c2.disconnect();

		if(verbose)
		{
			std::cout << "Discovered addresses:" << std::endl;
			for(std::set<boost::asio::ip::address_v6>::iterator it = addresses.begin(); it != addresses.end(); ++it)
			{
				std::cout << "\t" << it->to_string() << std::endl;
			}
			std::cout << std::endl;
		}
	} else if(vm.count("ip")) {
		addresses.insert(boost::asio::ip::address_v6::from_string(vm["ip"].as<std::string>()));
	} else {
		std::cerr << "You must either specify an IP address or use the --discover option." << std::endl;
		exit(1);
	}

	for(std::set<boost::asio::ip::address_v6>::iterator address_it = addresses.begin(); address_it != addresses.end(); ++address_it)
	{
		device_descriptor device;
		boost::asio::ip::address_v6 target_ip = *address_it;
		device.ipv6_address = target_ip;

		if(verbose)
			std::cout << "Querying device " << target_ip.to_string() << "..." << std::endl;

		ReportCallback reportCallback;
		PropreportCallback propreportCallback;
		TransmissionCallback transmissionCallback = { &(network->ioService()) };
		ErrorCallback errorCallback;
		NameCallback nameCallback = { &device };

		boost::signals2::connection c1 = network->onAsyncError(errorCallback);

		//Query device name
		boost::signals2::connection c2 = network->onPacketReceived(boost::ref(nameCallback),
			hexabus::filtering::isReport<std::string>());
		network->onPacketTransmitted(boost::ref(transmissionCallback),
			hexabus::QueryPacket(EP_DEVICE_DESCRIPTOR, hexabus::Packet::want_ack),
			boost::asio::ip::udp::endpoint(target_ip, HXB_PORT));

		network->ioService().reset();
		network->ioService().run();
		c2.disconnect();

		//Query all endpoints
		propreportCallback.next_eid = 0;
		do {
			endpoint_descriptor current_ep;
			current_ep.eid = propreportCallback.next_eid;
			reportCallback.current_ep = &current_ep;
			propreportCallback.current_ep = &current_ep;

			if(verbose)
				std::cout<< "\tQuerying Endpoint " << propreportCallback.next_eid << std::endl;

			boost::signals2::connection c2 = network->onPacketReceived(boost::ref(reportCallback),
				hexabus::filtering::isEndpointReport());
			network->onPacketTransmitted(boost::ref(transmissionCallback),
				hexabus::EndpointQueryPacket(current_ep.eid, hexabus::Packet::want_ack),
				boost::asio::ip::udp::endpoint(target_ip, HXB_PORT));

			network->ioService().reset();
			network->ioService().run();
			c2.disconnect();

			//Get endpoint properties
			propreportCallback.current_pid = 0;
			boost::signals2::connection c3 = network->onPacketReceived(boost::ref(propreportCallback),
				hexabus::filtering::isPropertyReport<bool>() ||
				hexabus::filtering::isPropertyReport<uint8_t>() ||
				hexabus::filtering::isPropertyReport<uint32_t>() ||
				hexabus::filtering::isPropertyReport<float>() ||
				hexabus::filtering::isPropertyReport<std::string>() ||
				hexabus::filtering::isPropertyReport<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >() ||
				hexabus::filtering::isPropertyReport<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >());

			do {
				if(verbose)
					std::cout<< "\t\tQuerying Property " << propreportCallback.current_pid << std::endl;

				network->onPacketTransmitted(boost::ref(transmissionCallback),
					hexabus::PropertyQueryPacket(propreportCallback.current_pid, current_ep.eid, hexabus::Packet::want_ack),
					boost::asio::ip::udp::endpoint(target_ip, HXB_PORT));

				network->ioService().reset();
				network->ioService().run();

				if(!vm.count("metadata"))
				{
					break;
				}

			} while(propreportCallback.current_pid != 0);
			c3.disconnect();

			device.endpoint_ids.insert(current_ep);

		} while(propreportCallback.next_eid != 0);

		c1.disconnect();

		if(vm.count("print"))
		{
			// print the information onto the command line
			print_dev_info(device);
			std::cout << std::endl;
			for(std::set<endpoint_descriptor>::iterator it = device.endpoint_ids.begin(); it != device.endpoint_ids.end(); ++it)
			{
				if (device.endpoint_ids.count(*it))
				{
					print_ep_info(*it);
					if(vm.count("metadata")) {
						for(std::set<property_descriptor>::iterator pit = (*it).properties.begin(); pit != (*it).properties.end(); ++pit)
							if(pit->propid != 0) //omit 'next eid' property
								print_prop_info(*pit);
					}
				}
			}
		}

		devices.insert(device);
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
		ofs.open(vm["epfile"].as<std::string>().c_str(), std::fstream::app);
		if(!ofs)
		{
			std::cerr << "Error: Could not open output file: " << vm["epfile"].as<std::string>().c_str() << std::endl;
			return 1;
		}

		for(std::set<device_descriptor>::iterator it = devices.begin(); it != devices.end(); ++it)
		{
			for(std::set<endpoint_descriptor>::iterator itt = it->endpoint_ids.begin(); itt != it->endpoint_ids.end(); ++itt)
			{
				if (it->endpoint_ids.count(*itt))
				{
					write_ep_desc(*itt, ofs);
				}
			}
		}
	}

	if(vm.count("devfile"))
	{
		if(vm.count("json"))
		{
			std::ofstream ofs;

			if(vm["devfile"].as<std::string>()=="-") {
				// write to stdout instead of a file
			} else {
				// open file
				ofs.open(vm["devfile"].as<std::string>().c_str(), std::fstream::out);
			}

			std::ostream& out(vm["devfile"].as<std::string>() == "-" ? std::cout : ofs);

			if(!out)
			{
				std::cerr << "Error: Could not open output file: " << vm["devfile"].as<std::string>().c_str() << std::endl;
				return 1;
			}

			out << "{\"devices\": [" << std::endl;

			bool hasWritten = false;
			for(std::set<device_descriptor>::iterator it = devices.begin(); it != devices.end(); ++it)
			{
				hasWritten |= write_dev_desc_json(*it, out, hasWritten && it != devices.begin(), vm.count("metadata"));
			}

			out << "]}" << std::endl;

		} else {
			hexabus::hbc_doc hbc_input = read_file(vm["devfile"].as<std::string>(), verbose);

			std::set<boost::asio::ip::address_v6> existing_dev_addresses;
			// check all the device definitions, and store their ip addresses in the set.
			BOOST_FOREACH(hexabus::hbc_block block, hbc_input.blocks)
			{
				if(block.which() == 2) // alias_doc
				{
					BOOST_FOREACH(hexabus::alias_cmd_doc ac, boost::get<hexabus::alias_doc>(block).cmds)
					{
						if(ac.which() == 0) // alias_ip_doc
						{
							existing_dev_addresses.insert(
									boost::asio::ip::address_v6::from_string(
										boost::get<hexabus::alias_ip_doc>(ac).ipv6_address));
						}
					}
				}
			}

			if(verbose)
			{
				std::cout << "Device IPs already present in file: " << std::endl;
				for(std::set<boost::asio::ip::address_v6>::iterator it = existing_dev_addresses.begin();
						it != existing_dev_addresses.end();
						++it)
				{
					std::cout << "\t" << it->to_string() << std::endl;
				}
			}

			// open file
			std::ofstream ofs;
			ofs.open(vm["devfile"].as<std::string>().c_str(), std::fstream::app);
			if(!ofs)
			{
				std::cerr << "Error: Could not open output file: " << vm["devfile"].as<std::string>().c_str() << std::endl;
				return 1;
			}

			for(std::set<device_descriptor>::iterator it = devices.begin(); it != devices.end(); ++it)
			{

				// only insert our device descriptors if the IP is not already found.
				if(!existing_dev_addresses.count(it->ipv6_address))
					write_dev_desc(*it, ofs);
			}
		}
	}
}
