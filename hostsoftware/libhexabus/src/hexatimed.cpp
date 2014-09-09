#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <libhexabus/common.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <libhexabus/socket.hpp>
#include "../../../shared/endpoints.h"
#include "../../../shared/hexabus_definitions.h"

namespace po = boost::program_options;
namespace ba = boost::asio;
namespace pt = boost::posix_time;

bool verbose = false;

enum ErrorCode {
	ERR_NONE = 0,

	ERR_UNKNOWN_PARAMETER = 1,
	ERR_PARAMETER_MISSING = 2,
	ERR_PARAMETER_FORMAT = 3,
	ERR_PARAMETER_VALUE_INVALID = 4,
	ERR_NETWORK = 5,

	ERR_OTHER = 127
};

void sendTime(const boost::system::error_code& e, ba::deadline_timer& t, unsigned int delay, hexabus::Socket& network, ba::ip::address_v6& hxb_broadcast_address) {
	pt::ptime currentTime(pt::second_clock::local_time());

	try {
		network.send(hexabus::TimeInfoPacket(currentTime), hxb_broadcast_address);
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Could not send packet to " << hxb_broadcast_address << ": " << e.code().message() << std::endl;
	}

	t.expires_at(t.expires_at() + pt::seconds(delay));
	t.async_wait(boost::bind(sendTime, ba::placeholders::error, boost::ref(t), delay, boost::ref(network), boost::ref(hxb_broadcast_address)));

	if(verbose) {
		std::cout << "Updated time to: " << pt::to_simple_string(currentTime) << std::endl;
	}
}

int main(int argc, char** argv)
{
	// the command line interface
	std::ostringstream oss;
	oss << "Usage: " << argv[0] << " [options]";
	po::options_description desc(oss.str());
	desc.add_options()
		("help,h", "produce help message")
		("version", "print version and exit")
		("interface,I", po::value<std::string>(), "outgoing interface")
		("delay,d", po::value<unsigned int>(), "delay between updates in seconds (default 60)")
		("verbose,V", "print more status information")
		;

	po::variables_map vm;

	// begin processing of command line parameters
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);
	} catch (std::exception& e) {
		std::cerr << "Cannot process command line: " << e.what() << std::endl;
		return ERR_UNKNOWN_PARAMETER;
	}

	if(vm.count("help"))
	{
		std::cout << desc << std::endl;
		return ERR_NONE;
	}

	if(vm.count("version"))
	{
		std::cout << "hexatimed -- hexabus date time broadcast daemon" << std::endl;
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		return ERR_NONE;
	}

	if(!vm.count("interface"))
	{
		std::cerr << "You must specify an interface to send mulitcasts from.";
		return ERR_PARAMETER_MISSING;
	}

	unsigned int delay = 60;
	if(vm.count("delay"))
		delay = vm["delay"].as<unsigned int>();

	if(vm.count("verbose"))
		verbose = true;

	//Setup interface
	ba::io_service io;
	ba::ip::address_v6 hxb_broadcast_address = ba::ip::address_v6::from_string(HXB_GROUP);

	hexabus::Socket network(io);
	try {
		network.mcast_from(vm["interface"].as<std::string>());
	} catch(const hexabus::NetworkException& e) {
		std::cerr << "Could not open interface "<< vm["interface"].as<std::string>() << ": " << e.code().message() << std::endl;
		return ERR_NETWORK;
	}

	if(verbose)
		std::cout << "Initialized network." << std::endl;

	ba::deadline_timer periodicTimer(io, pt::seconds(delay));
	periodicTimer.async_wait(boost::bind(sendTime, ba::placeholders::error, boost::ref(periodicTimer), delay, boost::ref(network), boost::ref(hxb_broadcast_address)));

	io.run();

	if(verbose)
		std::cout << "Closing..." << std::endl;

	return ERR_NONE;
}
