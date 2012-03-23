#include <iostream>
#include <libhexabus/packet.hpp>
#include <libhexabus/network.hpp>
#include "../../../shared/hexabus_packet.h"
#include <string>

void usage() {
	std::cout << "Usage: hexinfo <IPv6 Address of Device>" << std::endl;
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
	
	// Initialize network TODO: Open/Close Socket?!
	hexabus::NetworkAccess network;
	hexabus::Packet::Ptr pFactory(new hexabus::Packet());
	
	// Create a new query packet to endpoint 0
	hxb_packet_query packet = pFactory->query(0, false);
	
	// Send query to specified IP Address
	std::cout << "Sending Query..." << std::endl;
	network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
	// Waiting for related message
	network.receivePacket(true);
	// Extract data from recv. packet
	hexabus::PacketHandling *pHandler = new hexabus::PacketHandling(network.getData());
	hxb_value val = pHandler->getValue();
	delete pHandler;
	// Used Enpoints are stored in a 32bit vector where 1 indicates an used and 0 an unused endpoint
	std::cout << "Used Endpoints: " << std::endl;
	for(int i = 0;i < 32;i++) {
		if(*((uint32_t*)&(val.data)) & 1<<i) {
			// Endpoint is used, send a query to get datatype. TODO: Read/Write info in epquery?
			std::cout << "EID " << i + 1 << ":" << std::endl;
			packet = pFactory->query(i + 1, true);
			network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
			network.receivePacket(true);
			// Extract data
			pHandler = new hexabus::PacketHandling(network.getData());
			std::cout << "\t Endpoint Name: " << pHandler->getString() << std::endl;
			std::cout << "\t DataType: " << datatypeToString(pHandler->getDatatype()) << std::endl;
			delete pHandler;
		}
 	}
	std::cout << std::endl;

	return 0;
}
