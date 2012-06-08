#include <iostream>
#include <libhexabus/packet.hpp>
#include <libhexabus/network.hpp>
#include "../../../shared/hexabus_packet.h"
#include <string>
#include <boost/program_options.hpp>
#include "DeviceInfo.hpp"

namespace po = boost::program_options;

int main(int argc, char **argv) {
	std::ostringstream usage;
	usage << "Usage: " << argv[0] << " <IPv6 Address of Device> " << " [additional Options]" << std::endl;
	
	po::options_description desc(usage.str());
	desc.add_options()
		("help", "Print help message")
		("ipAddr", po::value<std::string>(), "IPv6 Address of Device")
	;
	po::positional_options_description pos;
	pos.add("ipAddr", 1);
	po::variables_map vm;
	
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(pos).run(), vm);
		po::notify(vm);
	} catch(std::exception& e) {
		std::cerr << "Could not parse command line: " << e.what() << std::endl;
	}
	
	if(vm.count("help")) {
		std::cout << desc << std::endl;
		return 0;
	}

	if(!vm.count("ipAddr")) {
		std::cerr << "IPv6 Address is missing!" << std::endl << usage.str();
		return 1;
	}
	
	std::vector<EndpointInfo> eps;
	try {
		DeviceInfo devInfo(vm["ipAddr"].as<std::string>());
		eps = devInfo.getAllEndpointInfo();
	} catch(std::invalid_argument& e) {
		std::cerr << e.what() << std::endl << usage.str();
		return 1;
	} catch(std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	
	// Why C++ needs for each...
	for (int i = 0;i < (int)eps.size();i++) {
 		std::cout << eps[i].toString(false);	// No Json for now
	}
	std::cout << std::endl;

	return 0;
}
