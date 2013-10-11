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

#define BAUD 	57600
#define VENID	0x24ad
#define PRODID	0x008e

#define P_ON_STR "$HXB_P_ON&"
#define P_OFF_STR "$HXB_P_OFF&"
#define P_SUC_STR "$HXB_P_SUCCESS&"
#define P_FAL_STR "$HXB_P_FAILED&"

namespace po = boost::program_options;
namespace ba = boost::asio;

bool verbose = false;

class hxb_serial {
public:
	hxb_serial(std::string devnode, unsigned int baudrate) {
		io_thread = new boost::thread(&hxb_serial::run_io, this);
		setup_serial(devnode, baudrate);
		initiate_pairing();
		start_reading();
	}

	hxb_serial(std::string devnode, unsigned int baudrate, unsigned int timeout_time) {
		boost::thread(&hxb_serial::run_io, this);
		setup_serial(devnode, baudrate);
		setup_timer(timeout_time);
		initiate_pairing();
		initiate_timeout();
		start_reading();
	}
	
private:
	void setup_serial(std::string devnode, unsigned int baudrate) {
		
		dev = new ba::serial_port(io);

		try {
			dev->open(devnode);
		} catch (std::exception& e) {
			std::cerr << "Could not open serial device: " << e.what() << std::endl;
			exit(-1);
		}
		dev->set_option(ba::serial_port_base::baud_rate(baudrate));

		cmd_buf.clear();
	}

	void setup_timer(unsigned int timeout_time) {
		this->timeout_time = timeout_time;
		timeout_timer = new ba::deadline_timer(io, boost::posix_time::seconds(timeout_time));
	}

	void initiate_pairing() {
		std::string sstart = P_ON_STR;
		ba::write(*dev, ba::buffer(sstart.c_str(),sstart.size()));
	}

	void run_io() {
		io.run();
	}

	void timeout_reached() {
		std::cout << "Timeout!" << std::endl;
		exit(-1);
		return;
	}

	void initiate_timeout() {
		//timeout_timer->wait();
		timeout_timer->async_wait(boost::bind(&hxb_serial::timeout_reached, this));
	}

	void start_reading() {
		ba::async_read(*dev, ba::buffer(read_buf), boost::bind(&hxb_serial::read_char, this));
	}

	int parse_cmd() {

		if(verbose)
			std::cout << "Testing: " << cmd_buf << std::endl;

		if(!cmd_buf.compare(P_SUC_STR)) {
			std::cout << "Success!" << std::endl;
			return 1;
		} else if(!cmd_buf.compare(P_FAL_STR)) {
			std::cout << "Failed!" << std::endl;
			return 1;
		} else {
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
			if (parse_cmd())
				return;
		} else {
			cmd_buf.push_back(read_buf[0]);
		}

		start_reading();
	}

	boost::thread* io_thread;
	ba::io_service io;
	ba::serial_port* dev;
	boost::array<char, 1> read_buf;
	std::string cmd_buf;
	unsigned int timeout_time;
	ba::deadline_timer* timeout_timer;
	std::ostringstream output;

	//char read_buf[1];
};


void expi() {
	std::cout << "Expired" << std::endl;
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

/*
	if(vm.count("timeout")) {
		hxb_serial stick(devnode, baud, vm["timeout"].as<unsigned int>());
	} else {
		hxb_serial stick(devnode, baud);
	}
*/

/*
	ba::io_service io;
	ba::deadline_timer t1(io);
	ba::deadline_timer t2(io);
	t1.expires_from_now(boost::posix_time::seconds(5));
	t2.expires_from_now(boost::posix_time::seconds(10));
	t1.async_wait(boost::bind(&expi));
	t2.async_wait(boost::bind(&expi));
	
	io.run();
*/

	// io.stop();
	/*
	serial_dev.close();
	if(verbose)
		std::cout << "Closing..." << std::endl;
	
	while(1) { 
		std::cout << "Fooo";
		sleep(1);
	}
	*/
	return 0;
}
