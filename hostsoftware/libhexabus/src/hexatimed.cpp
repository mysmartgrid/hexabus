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

bool verbose = false;

class DatetimeSupply {
	public:
		DatetimeSupply(ba::io_serivce& io) {

		}

	private:


};

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
		("nofork,n", "dont't fork")
		("verbose,V", "print more status information")
		;

	po::variables_map vm;
	bool fork = true;

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
		std::cout << "hexatimed -- hexabus date time broadcast daemon" << std::endl;
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		return 0;
	}

	if(vm.count("verbose"))
		verbose = true;

	if(vm.count("nofork"))
		fork = false;

	try {
		ba::io_serivce io;
		DatetimeSupply supply(io);

		ba::signal_set signals(io, SIGINT, SIGTERM);
    	signals.async_wait(boost::bind(&ba::io_service::stop, &io));

    	io.notify_fork(boost::asio::io_service::fork_prepare);

    	if (pid_t pid = fork()) {
    		if (pid > 0) {
    			exit(0);
    		} else {
				syslog(LOG_ERR | LOG_USER, "Forking failed: %m");
				return 1;
    		}
    	}

		setsid();
		chdir("/");

		if (pid_t pid = fork()) {
			if (pid > 0) {
				exit(0);
			} else {
				syslog(LOG_ERR | LOG_USER, "Forking failed: %m");
				return 1;
			}
		}

		close(0);
		close(1);
		close(2);

		io.notify_fork(boost::asio::io_service::fork_child);

		syslog(LOG_INFO | LOG_USER, "hexatimed started");
		io.run();
		syslog(LOG_INFO | LOG_USER, "hexatimed stopped");	
	} catch (std::exception& e) {
		std::cerr << "Failed to start daemon process: " << e.what() << std::endl;
	}

	if(verbose)
		std::cout << "Closing..." << std::endl;

	return 0;
}
