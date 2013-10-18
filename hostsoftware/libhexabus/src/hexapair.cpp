#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <libusb-1.0/libusb.h>
#include <libhexabus/common.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <libhexabus/socket.hpp>
#include "../../../shared/endpoints.h"
#include "../../../shared/hexabus_definitions.h"

#define BAUD 	57600
#define VENID	0x24ad
#define PRODID	0x008e

#define P_ON_STR "$HXB_P_ON&"
#define P_OFF_STR "$HXB_P_OFF&"
#define P_SUC_STR "$HXB_P_SUCCESS&"
#define P_FAL_STR "$HXB_P_FAILED&"

#define RESPONSE_TIMEOUT 5

namespace po = boost::program_options;
namespace ba = boost::asio;

bool verbose = false;

struct rcv_callback {
	ba::ip::address_v6* ip;
	ba::io_service* io;
	void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep)
	{
		*ip = asio_ep.address().to_v6();
		io->stop();
	}
};

class hxb_serial {
public:
	hxb_serial(ba::serial_port* sdev, ba::io_service* io) {
		this->dev = sdev;
		this->io = io;
		cmd_buf.clear();
	}
	
	void initiate_pairing() {
		std::string sstart = P_ON_STR;
		ba::write(*dev, ba::buffer(sstart.c_str(),sstart.size()));
	}

	void cancel_pairing() {
		std::string sstop = P_OFF_STR;
		ba::write(*dev, ba::buffer(sstop.c_str(),sstop.size()));
	}

	void start_reading() {
		ba::async_read(*dev, ba::buffer(read_buf), boost::bind(&hxb_serial::read_char, this));
	}

	int get_return() {
		return return_val;
	}

	ba::io_service* io;

private:
	int parse_cmd() {

		if(verbose)
			std::cout << "Read: " << cmd_buf << std::endl;

		if(!cmd_buf.compare(P_SUC_STR)) {
			if(verbose)
				std::cout << "Success!" << std::endl;
			cmd_buf.clear();
			return 1;
		} else if(!cmd_buf.compare(P_FAL_STR)) {
			if(verbose)
				std::cout << "Failed!" << std::endl;
			cmd_buf.clear();
			return -1;
		} else {
			if(verbose)
				std::cout << "Unknown command" << std::endl;
		}

		cmd_buf.clear();
		return 0;
	}

	void read_char() {
		if(read_buf[0] == '$') {
			cmd_buf.clear();
			cmd_buf.push_back(read_buf[0]);
		} else if(read_buf[0] == '&') {
			cmd_buf.push_back(read_buf[0]);
			return_val = parse_cmd();
			if (return_val) {
				io->stop();
				return;
			}
		} else {
			cmd_buf.push_back(read_buf[0]);
		}

		start_reading();
	}

	ba::serial_port* dev;
	boost::array<char, 1> read_buf;
	std::string cmd_buf;
	unsigned int timeout_time;
	int return_val;
};


void response_timeout(const boost::system::error_code& e, hxb_serial* stick) {
	if(!e) {
		std::cerr << "Timeout, hexabus stick did not respond." << std::endl;
		stick->io->stop();
		exit(-1);
	}
}


void stop_pairing(hxb_serial *stick) {
	ba::deadline_timer response_timer(*(stick->io));

	stick->io->reset();

	if(verbose)
		std::cout << "Canceling running pairing" << std::endl;

	stick->cancel_pairing();
	stick->start_reading();

	response_timer.expires_from_now(boost::posix_time::seconds(RESPONSE_TIMEOUT));
	response_timer.async_wait(boost::bind(&response_timeout, ba::placeholders::error, stick));

	stick->io->run();
	response_timer.cancel();
	
	if(stick->get_return() == -1) {
		std::cerr << "Could not cancel previous pairing." << std::endl;
		exit(-1);
	}
}

void start_pairing(hxb_serial *stick) {
	ba::deadline_timer response_timer(*(stick->io));

	stick->io->reset();

	if(verbose)
		std::cout << "Initiating new pairing" << std::endl;

	stick->initiate_pairing();
	stick->start_reading();

	response_timer.expires_from_now(boost::posix_time::seconds(RESPONSE_TIMEOUT));
	response_timer.async_wait(boost::bind(&response_timeout, ba::placeholders::error, stick));

	stick->io->run();
	response_timer.cancel();

	if(stick->get_return() == -1) {
		std::cerr << "Could not initiate pairing." << std::endl;
		exit(-1);
	}
}

void timeout(const boost::system::error_code& e, hxb_serial* stick) {
	if(!e) {
		std::cerr << "Timeout, no device found." << std::endl;
		stick->io->stop();
		stop_pairing(stick);
		exit(-1);
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
		("timeout,t", po::value<unsigned int>(), "Pairing timeout in seconds")
		("stop,s", "Cancel a running pairing process")
		("device,D", po::value<std::string>(), "Use a specific serial device")
		("baud,B", po::value<unsigned int>(), "Set the baud rate for the serial device.")
		("noinfo,n", "Do not ask for device info after pairing is finished")
		("verbose,V", "print more status information")
		;

	po::variables_map vm;

	// begin processing of command line parameters
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
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
		std::cout << "hexapair -- hexabus pairing utility" << std::endl;
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		return 0;
	}

	if(vm.count("verbose"))
		verbose = true;

	//set up serial device

	unsigned int baud = BAUD;

	if(vm.count("baud"))
		baud = vm["baud"].as<unsigned int>();

	std::string devnode = "/dev/ttyACM0";

	if(vm.count("device"))
		devnode = vm["device"].as<std::string>();

	ba::io_service io;
	ba::serial_port sdev(io);
	ba::deadline_timer timeout_timer(io);

	try {
		sdev.open(devnode);
	} catch (std::exception& e) {
		std::cerr << "Could not open serial device: " << e.what() << std::endl;
		exit(-1);
	}

	sdev.set_option(ba::serial_port_base::baud_rate(baud));

	hxb_serial stick(&sdev, &io);
	
	hexabus::Socket* network;

	if(!vm.count("noinfo")) {
		//Initialize network

		try {
			network = new hexabus::Socket(io);
		} catch(const hexabus::NetworkException& e) {
			std::cerr << "Could not open socket: " << e.code().message() << std::endl;
			return 1;
		}

		ba::ip::address_v6 bind_addr(ba::ip::address_v6::any());
		network->listen();
	}

	//TODO flush buffer

	//cancel possible running pairing processes
	stop_pairing(&stick);

	//initiate new pairing
	if(!vm.count("stop")) {

		start_pairing(&stick);

		if(vm.count("timeout")) {
			io.reset();

			if(verbose)
				std::cout << "Waitung for pairing to complete" << std::endl;

			timeout_timer.expires_from_now(boost::posix_time::seconds(vm["timeout"].as<unsigned int>()));
			timeout_timer.async_wait(boost::bind(&timeout, ba::placeholders::error, &stick));

			if(!vm.count("noinfo")) {
				ba::ip::address_v6 ip;
				rcv_callback cb = {&ip, &io};
				boost::signals2::connection con = network->onPacketReceived(cb, hexabus::filtering::isInfo<uint32_t>() && (hexabus::filtering::eid() == EP_DEVICE_DESCRIPTOR));
				io.run();
				con.disconnect();
				if(verbose)
					std::cout << "Found device: ";
				std::cout << ip.to_string() << std::endl;
			} else {
				stick.start_reading();
				io.run();
			}

			timeout_timer.cancel();

			if(stick.get_return() == -1) {
				std::cerr << "Pairing failed." << std::endl;
				exit(-1);
			}

			stop_pairing(&stick);
		}
	}
	
	sdev.close();

	if(verbose)
		std::cout << "Closing..." << std::endl;

	return 0;
}
