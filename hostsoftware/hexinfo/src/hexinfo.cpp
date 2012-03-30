// TODO:
// 1. Validation
// 2. JSON Export
// 3. Testing

#include <iostream>
#include <libhexabus/packet.hpp>
#include <libhexabus/network.hpp>
#include "../../../shared/hexabus_packet.h"
#include <string>

// A vector with used EIDs contains 32 values
const int EIDS_PER_VECTOR = 32;

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
	
	// Initialize network TODO: Open/Close Socket?!
	hexabus::NetworkAccess network;
	hexabus::Packet::Ptr pFactory(new hexabus::Packet());
	
	// MaxEID: Maximum EID of the specific device. This value is updated at run time, because the current eid usage vector indicates whether there are more eids to come.
	// At the beginning we assume, that there are only 32 used EIDs.
	int maxEID = EIDS_PER_VECTOR;
	
	char *ipAddress = argv[1];
	
	// Create a new query packet to endpoint 0
	hxb_packet_query packet = pFactory->query(0, false);
	
	// Send query to specified IP Address
	std::cout << "Sending Query..." << std::endl;
	network.sendPacket(ipAddress, HXB_PORT, (char*)&packet, sizeof(packet));
	network.receivePacket(true);
	
	// Extract data from recv. packet
	// Used Enpoints are stored in a EIDS_PER_VECTOR-bit vector where 1 indicates an used and 0 an unused endpoint
	hexabus::PacketHandling *pHandler = new hexabus::PacketHandling(network.getData());
	hxb_value val = pHandler->getValue();
	uint32_t eidVector = *((uint32_t*)&(val.data));
	delete pHandler;
	std::cout << "Used Endpoints: " << std::endl;
	
	for(int i = 0;i < maxEID;i++) {
		// Check, if current index matches an used EID
		if(eidVector & 1<<i) {
			if(i + 1 == maxEID) {
				// This is a special case: There are more endpoints with EID > maxEID ~> we need to get another vector.
				maxEID += EIDS_PER_VECTOR;
				packet = pFactory->query(i + 1, false);
				network.sendPacket(ipAddress, HXB_PORT, (char*)&packet, sizeof(packet));
				network.receivePacket(true);
				// Extract next used EID vector
				pHandler = new hexabus::PacketHandling(network.getData());
				val = pHandler->getValue();
				eidVector = *((uint32_t*)&(val.data));
				delete pHandler;
			} else {
				// Endpoint is used, send a query to get datatype. TODO: Read/Write info in epquery?
				std::cout << "EID " << i + 1 << ":" << std::endl;
				packet = pFactory->query(i + 1, true);
				network.sendPacket(ipAddress, HXB_PORT, (char*)&packet, sizeof(packet));
				network.receivePacket(true);
				// Extract data
				pHandler = new hexabus::PacketHandling(network.getData());
				std::cout << "\t Endpoint Name: " << pHandler->getString() << std::endl;
				std::cout << "\t DataType: " << datatypeToString(pHandler->getDatatype()) << std::endl;
				delete pHandler;
			}
		}
 	}
	std::cout << std::endl;

	return 0;
}
