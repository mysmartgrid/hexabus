#include <iostream>
#include <libhexabus/packet.hpp>
#include <libhexabus/network.hpp>
#include "../../../shared/hexabus_packet.h"
#include <string>
#include <boost/program_options.hpp>
#include "DeviceInfo.hpp"
#include "config.h"

namespace po = boost::program_options;

int main(int argc, char **argv) {
	std::ostringstream usage;
	usage << "Usage: " << argv[0] << " <IPv6 Address of Device> " << " [additional Options]" << std::endl;
	bool json = false;
	
	po::options_description desc(usage.str());
	desc.add_options()
		("help,h", "Print help message")
		("version,v", "Prints the version and exits")
		("ipAddr", po::value<std::string>(), "IPv6 Address of Device")
		("json,j", "Enable JSON Output")
		("interface,i", po::value<std::string>(), "Interface to be used for communication")
	;
	po::positional_options_description pos;
	pos.add("ipAddr", 1);
	po::variables_map vm;
	
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(pos).run(), vm);
		po::notify(vm);
	} catch(std::exception& e) {
		std::cerr << "Could not parse command line: " << e.what() << std::endl << usage.str() << std::endl;
		return 1;
	}
	
	if(vm.count("help")) {
		std::cout << desc << std::endl;
		return 0;
	}

	if(vm.count("version")) {
		std::cout << "Hexinfo version: " << HEXINFO_VERSION_MAJOR << "." << HEXINFO_VERSION_MINOR << "." << HEXINFO_VERSION_PATCH << std::endl;
		return 0;
	}

	if(!vm.count("ipAddr")) {
		std::cerr << "IPv6 Address is missing!" << std::endl << usage.str();
		return 1;
	}
	
	if(vm.count("json")) {
		json = true;
	}
	
	std::vector<EndpointInfo> eps;
	DeviceInfo *devInfo;
	try {
		if(vm.count("interface")) {
			devInfo = new DeviceInfo(vm["ipAddr"].as<std::string>(), vm["interface"].as<std::string>());
		} else {
			devInfo = new DeviceInfo(vm["ipAddr"].as<std::string>());
		}
		eps = devInfo->getAllEndpointInfo();
	} catch(std::invalid_argument& e) {
		std::cerr << e.what() << std::endl << usage.str() << "Try --help for more information" << std::endl;
		return 1;
	} catch(std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	
	// Make it an array, so handling it elsewhere is easier
	if(json)
		std::cout << "{ \"Endpoints\" : [" << std::endl;

	for (int i = 0;i < (int)eps.size();i++) {
 		std::cout << eps[i].toString(json) << (json ? "," : "")  << std::endl;
	}
	if(json)
		std::cout << "] }" << std::endl;
	else
		std::cout << std::endl;

	return 0;
}
