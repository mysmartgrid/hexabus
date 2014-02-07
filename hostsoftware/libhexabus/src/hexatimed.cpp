#include <iostream>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/asio/io_serivce.hpp>
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

void sendTime(const boost::system::error_code& e, boost::asio::deadline_timer* t) {
	pt::ptime currentTime(pt::second_clock::local_time());
	send_packet(network, hxb_broadcast_address, hexabus::InfoPacket(0, currentTime));
	t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
	t->async_wait(boost::bind(sendTime, boost::asio::placeholders::error, t));

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
		("interface,I", po::value<std::vector<std::string> >(), "outgoing interface")
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
		std::cerr << "You must specify an interface to send mulitcasts from."
		return ERR_PARAMETER_MISSING;
	}

	if(vm.count("verbose"))
		verbose = true;

	//Setup interface
	ba::io_serivce io;
	boost::asio::ip::address_v6 hxb_broadcast_address = boost::asio::ip::address_v6::from_string(HXB_GROUP);
	try {
		hexabus::Socket* network = new hexabus::Socket(io, vm["interface"].as<std::string>());
	} catch(const hexabus::NetworkException& e) {
		std::cerr << "Colud not open socket: " << e.what().message() << std::endl;
		return ERR_NETWORK;
	}

	boost::asio::deadline_timer periodicTimer(io, boost::posix_time::seconds(1));
	periodicTimer.async_wait(boost::bind(sendTime, boost::asio::placeholders::error, &periodicTimer));

	boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
	signals.async_wait(boost::bind(&boost::asio::io_service::stop, &io));

	io.run();


	if(verbose)
		std::cout << "Closing..." << std::endl;

	return ERR_NONE;
}
