#include "DeviceInfo.hpp"
#include "../../shared/hexabus_packet.h"

DeviceInfo::DeviceInfo(std::string ipAddress) {
	this->ipAddress = ipAddress;
	this->pFactory = hexabus::Packet::Ptr(new hexabus::Packet());	// TODO
	// Initialize network TODO: Open/Close Socket?!
}

EndpointInfo DeviceInfo::getEndpointInfo(int eid) {
	// Create a query to the specified endpoint. TODO: EID 0.
	hxb_packet_query packet = pFactory->query(eid, true);
	
	network.sendPacket(ipAddress, HXB_PORT, (char*)&packet, sizeof(packet));
	network.receivePacket(true);
	
	// Extract Data
	hexabus::PacketHandling *pHandler = new hexabus::PacketHandling(network.getData());
	// TODO: Check for errors
	

	return EndpointInfo(eid, pHandler->getString(), "todo", pHandler->getDatatype(), false);
}

std::vector<int> DeviceInfo::getDeviceDescriptor() {
	std::vector<int> devDescriptor;
	return devDescriptor;
}

std::vector<EndpointInfo> DeviceInfo::getEndpointInfo(std::vector<int> eidList) {
	std::vector<EndpointInfo> endpoints;
	return endpoints;
}
