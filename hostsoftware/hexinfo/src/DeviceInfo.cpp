#include "DeviceInfo.hpp"
#include "../../shared/hexabus_packet.h"

// TODO: Error Handling
// What is open/close socket in libhexabus used for?


EndpointInfo::EndpointInfo(uint8_t eid, std::string name, std::string description, uint8_t datatype, bool writable) {
			this->eid = eid;
			this->name = name;
			this->description = description;
			this->datatype = datatype;
			this->writable = writable;
}

std::string EndpointInfo::getName() {return name;}

std::string EndpointInfo::getDescription() {return description;}

uint8_t EndpointInfo::getDatatype() {return datatype;}

bool EndpointInfo::isWritable() {return writable;}

std::string EndpointInfo::toString() {
	std::string tmpDT;
	switch(datatype) {
		case HXB_DTYPE_BOOL:
			tmpDT = "Bool";
			break;
		case HXB_DTYPE_UINT8:
			tmpDT = "UInt8";
			break;
		case HXB_DTYPE_UINT32:
			tmpDT = "UInt32";
			break;
		case HXB_DTYPE_DATETIME:
			tmpDT = "DateTime";
			break;
		case HXB_DTYPE_FLOAT:
			tmpDT = "Float";
			break;
		case HXB_DTYPE_128STRING:
			tmpDT = "String (128 Bit)";
			break;
		case HXB_DTYPE_TIMESTAMP:
			tmpDT = "Timestamp";
			break;
		default:
			tmpDT = "Unknown";
			break;
	}
	std::stringstream sstm;
	sstm << "EID: " << eid << " Name: " << name << " Description: " << description
		<< " Datatype: " << tmpDT << " Writable: ";
	if(writable)
		sstm << "Yes";
	else
		sstm << "No";
	return sstm.str();
}


DeviceInfo::DeviceInfo(std::string ipAddress) throw(std::invalid_argument) {
	this->ipAddress = ipAddress;
	// Validate input
	boost::system::error_code ec;
	boost::asio::ip::address::from_string(ipAddress, ec);
	if(ec) {
		network.closeSocket();
		throw std::invalid_argument(ec.message());
	}
	this->pFactory = hexabus::Packet::Ptr(new hexabus::Packet());
}

DeviceInfo::~DeviceInfo() {
	network.closeSocket();
}

EndpointInfo DeviceInfo::getEndpointInfo(int eid) {
	// Create a query to the specified endpoint. 
	hxb_packet_query packet = pFactory->query(eid, true);
	
	network.sendPacket(ipAddress, HXB_PORT, (char*)&packet, sizeof(packet));
	network.receivePacket(true);
	
	// Extract Data and check if the endpoint exists.
	// TODO: CRC Check, CRC Error and maybe defining a custom "NoSuchEndpoint" exception (with boost?)?
	
	hexabus::PacketHandling *pHandler = new hexabus::PacketHandling(network.getData());
	if(pHandler->getPacketType() == HXB_PTYPE_ERROR && pHandler->getErrorcode() == HXB_ERR_UNKNOWNEID) {
		throw std::runtime_error("No such endpoint");
	} else {
		// Everything went fine
		EndpointInfo epInfo(eid, pHandler->getString(), "test this", pHandler->getDatatype(), false);
		delete pHandler;
		return epInfo;
	}
	
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
	
	// For each would be nicer...
	for(int i = 0;i < (int)eidList.size();i++) {
		try {
			endpoints.push_back(getEndpointInfo(eidList[i]));
		} catch(std::runtime_error& e) {
			// For now just ignore the missing endpoint and continue
			continue;
		}
	}
	return endpoints;
}

std::vector<EndpointInfo> DeviceInfo::getAllEndpointInfo() {
	return getEndpointInfo(getDeviceDescriptor());
}
