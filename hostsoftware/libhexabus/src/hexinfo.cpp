#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <libhexabus/common.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <libhexabus/socket.hpp>
#include "../../../shared/endpoints.h"

namespace po = boost::program_options;

typedef std::pair<uint32_t, std::string> endpoint_descriptor;
typedef std::map<uint32_t, std::string> endpoint_map;

struct device_descriptor {
	boost::asio::ip::address_v6 ipv6_address;
	std::string name;
	std::vector<uint32_t> endpoint_ids;
};

struct ResponseHandler : public hexabus::PacketVisitor {
	public:
		ResponseHandler(device_descriptor& device, endpoint_map& endpoints) : _device(device), _endpoints(endpoints) {}

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
				ep.first = endpointInfo.eid();
				ep.second = endpointInfo.value();

				_endpoints.insert(ep);
			}
		}

		void visit(const hexabus::InfoPacket<uint32_t>& info)
		{
			if(info.eid() == 0) // we only care for the device descritor
			{
				uint32_t val = info.value();
				for(int i = 0; i < 32; ++i)
				{
					if(val % 2) // find out whether LSB is set
					{
						_device.endpoint_ids.push_back(i); // if it's set, store the EID (the bit's position in the device descriptor).
					}

					val >>= 1; // right-shift in order to have the next EID in the LSB
				}
			}
		}

	private:
		device_descriptor _device;
		endpoint_map _endpoints;
};

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
			// TODO here we assume that the first packet received from the right device is the one we're waiting for - is this correct?
			handler.visitPacket(*pair.first);
			break;
		}
	}
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
		("outfile,o", po::value<std::string>(), "name of Hexabus Compiler header file to produce")
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
		endpoint_map endpoints;
		ResponseHandler handler(device, endpoints);

		// send the device name query packet and listen for the reply
		send_packet(network, target_ip, hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR), handler);

		// send the device descriptor (endpoint list) query packet and listen for the reply
		send_packet(network, target_ip, hexabus::QueryPacket(EP_DEVICE_DESCRIPTOR), handler);

		// now, iterate over the endpoint list and find out the properties of each endpoint
		// TODO

	}

	if(vm.count("outfile"))
	{
		throw hexabus::GenericException("Not Implemented");
	}

}
