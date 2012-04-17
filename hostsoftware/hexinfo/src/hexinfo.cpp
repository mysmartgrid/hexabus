// TODO:
// 1. Validation
// 2. JSON Export
// 3. Testing

#include <iostream>
#include <libhexabus/packet.hpp>
#include <libhexabus/network.hpp>
#include "../../../shared/hexabus_packet.h"
#include <string>
#include "DeviceInfo.hpp"

void usage() {
	std::cout << "Usage: hexinfo <IPv6 Address of Device> <Options>" << std::endl;
	std::cout << "Possible options are:" << std::endl;
	std::cout << "-j \t output json instead of the normal output" << std::endl;
}

std::string datatypeToString(uint8_t datatype) {
	switch(datatype) {
		case HXB_DTYPE_BOOL:
			return "Bool";
		case HXB_DTYPE_UINT8:
			return "UInt8";
		case HXB_DTYPE_UINT32:
			return "UInt32";
		case HXB_DTYPE_DATETIME:
			return "DateTime";
		case HXB_DTYPE_FLOAT:
			return "Float";
		case HXB_DTYPE_128STRING:
			return "String (128 Bit)";
		case HXB_DTYPE_TIMESTAMP:
			return "Timestamp";
		default:
			return "Unkown";
	}
}

int main(int argc, char **argv) {
	
	if(argc != 2) {
		usage();
		//exit(1);
		return 1;
	}
	
	std::string ipAddress = argv[1];

	hexabus::NetworkAccess network;
	hexabus::Packet::Ptr pFactory(new hexabus::Packet());
	hexabus::PacketHandling *pHandler;
	hxb_packet_query packet;
	DeviceInfo devInfo(argv[1]);	
	// Get the available endpoints
	std::vector<int> endpoints = 	devInfo.getDeviceDescriptor();	
		
	// Why C++ needs for each...
	for (int i = 0;i < (int)endpoints.size();i++) {
		// Send a query to get endpoint data. TODO: Read/Write info in epquery?
		std::cout << "EID " << endpoints[i] << ":" << std::endl;
		packet = pFactory->query(endpoints[i], true);
		network.sendPacket(ipAddress, HXB_PORT, (char*)&packet, sizeof(packet));
		network.receivePacket(true);
		// Extract data
		pHandler = new hexabus::PacketHandling(network.getData());
		std::cout << "\t Endpoint Name: " << pHandler->getString() << std::endl;
		std::cout << "\t DataType: " << datatypeToString(pHandler->getDatatype()) << std::endl;
		delete pHandler;
 	}
	std::cout << std::endl;

	return 0;
}
