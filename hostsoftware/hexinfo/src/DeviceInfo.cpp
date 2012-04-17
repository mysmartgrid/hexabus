#include "DeviceInfo.hpp"
#include "../../shared/hexabus_packet.h"

// TODO: Validation/Error Handling
// What is open/close socket in libhexabus used for?

DeviceInfo::DeviceInfo(std::string ipAddress) {
	this->ipAddress = ipAddress;
	this->pFactory = hexabus::Packet::Ptr(new hexabus::Packet());	// TODO
}

EndpointInfo DeviceInfo::getEndpointInfo(int eid) {
	// Create a query to the specified endpoint. TODO: EID 0?
	hxb_packet_query packet = pFactory->query(eid, true);
	
	network.sendPacket(ipAddress, HXB_PORT, (char*)&packet, sizeof(packet));
	network.receivePacket(true);
	
	// Extract Data
	hexabus::PacketHandling *pHandler = new hexabus::PacketHandling(network.getData());

	return EndpointInfo(eid, pHandler->getString(), "todo", pHandler->getDatatype(), false);
}

std::vector<int> DeviceInfo::getDeviceDescriptor() {
	std::vector<int> devDescriptor;
	int maxEID = EIDS_PER_VECTOR;

	// Query eid 0 for the first device descriptor. Repeat until there is no further descriptor available
	hxb_packet_query query = pFactory->query(0, false);
	network.sendPacket(ipAddress, HXB_PORT, (char*)&query, sizeof(query));
	network.receivePacket(true);

	// Extract data from recv. packet
	// Enpoints are stored in a EIDS_PER_VECTOR-bit vector where 1 indicates an used and 0 an unused endpoint.
	hexabus::PacketHandling *pHandler = new hexabus::PacketHandling(network.getData());
	hxb_value val = pHandler->getValue();
	uint32_t eidVector = *((uint32_t*)&(val.data));
	delete pHandler;
	
	for(int i = 0;i < maxEID;i++) {
		if(eidVector & 1<<i) {
			// Endpoint is used ~> Add it to our List
			devDescriptor.push_back(i + 1);
			
			if(i + 1 == maxEID) {
				
				// There are more endpoints
				maxEID += EIDS_PER_VECTOR;
				query = pFactory->query(i + 1, false);
				network.sendPacket(ipAddress, HXB_PORT, (char*)&query, sizeof(query));
				network.receivePacket(true);
				
				// Extract the next vector
				pHandler = new hexabus::PacketHandling(network.getData());
				val = pHandler->getValue();
				eidVector = *((uint32_t*)&(val.data));
				delete pHandler;
			}
		}
	}

	return devDescriptor;
}

std::vector<EndpointInfo> DeviceInfo::getEndpointInfo(std::vector<int> eidList) {
	std::vector<EndpointInfo> endpoints;
	return endpoints;
}
