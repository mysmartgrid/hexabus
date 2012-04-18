#ifndef DEVICE_INFO_HPP
#define DEVICE_INFO_HPP

#include <string>
#include <sstream>
#include <libhexabus/network.hpp>
#include <libhexabus/packet.hpp>
#include "../../shared/hexabus_packet.h"

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
		EndpointInfo(uint8_t eid, std::string name, std::string description, uint8_t datatype, bool writable);
		std::string getName();
		std::string getDescription();
		uint8_t getDatatype();
		bool isWritable();
		std::string toString();
};

class DeviceInfo {
	private:
		std::string ipAddress;
		hexabus::NetworkAccess network;
		hexabus::Packet::Ptr pFactory;

	public:
		DeviceInfo(std::string ipAddress) throw (std::invalid_argument);
		~DeviceInfo();
		EndpointInfo getEndpointInfo(int eid);
		std::vector<int> getDeviceDescriptor();
		std::vector<EndpointInfo> getEndpointInfo(std::vector<int> eidList);
		std::vector<EndpointInfo> getAllEndpointInfo();
};


#endif
