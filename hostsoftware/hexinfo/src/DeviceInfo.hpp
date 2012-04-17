#ifndef DEVICE_INFO_HPP
#define DEVICE_INFO_HPP

#include <string>
#include <sstream>
#include <libhexabus/network.hpp>
#include <libhexabus/packet.hpp>

// A vector width used EIDs contains 32 values
const int EIDS_PER_VECTOR = 32;

class EndpointInfo {
	private:
		uint8_t eid;
		std::string name;
		uint8_t datatype;
		std::string description;
		bool writable;

	public:
		EndpointInfo(uint8_t eid, std::string name, std::string description, uint8_t datatype, bool writable) {
		
		}

		std::string getName() {return name;}
		std::string getDescription() {return description;}
		uint8_t getDatatype() {return datatype;}
		bool isWritable() {return writable;}
		std::string toString() {
			std::string tmpDT;
			switch(datatype) {
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
};

class DeviceInfo {
	private:
		std::string ipAddress;
		hexabus::NetworkAccess network;
		hexabus::Packet::Ptr pFactory;

	public:
		DeviceInfo(std::string ipAddress);
		EndpointInfo getEndpointInfo(int eid);
		std::vector<int> getDeviceDescriptor();
		std::vector<EndpointInfo> getEndpointInfo(std::vector<int> eidList);
		//std::string getEndpointInfoAsJSON(int startEID, int endEID);

};


#endif
