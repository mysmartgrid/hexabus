#include "solar_calculator.hpp"
#include <libhexabus/packet.hpp>
#include "../../../../shared/endpoints.h"

using namespace hexanode;

void SolarCalculator::push_value(const uint32_t eid, 
    const uint8_t value)
{
  std::cout << "Received EID " << eid
    << " value " << (uint16_t) value << std::endl;

  uint32_t current_production = value * (_peak_watt/255);
  std::cout << "Sending calculated production: " << current_production << std::endl;
  _send_socket->send(
      hexabus::InfoPacket<uint32_t>(EP_PV_PRODUCTION, current_production));
}


void SolarCalculator::printValueHeader(uint32_t eid, const char* datatypeStr)
{
  std::cout << " EID " << eid;
}

void SolarCalculator::printValuePacket(
    const hexabus::ValuePacket<uint8_t>& packet, const char* datatypeStr)
{
  push_value(packet.eid(), packet.value());
}

void SolarCalculator::visit(const hexabus::ErrorPacket& error)
{
  std::cout << "Received error packet, " << std::endl
    << "Error code:\t";
  switch (error.code()) {
    case HXB_ERR_UNKNOWNEID:
      std::cout << "Unknown EID";
      break;
    case HXB_ERR_WRITEREADONLY:
      std::cout << "Write on readonly endpoint";
      break;
    case HXB_ERR_CRCFAILED:
      std::cout << "CRC failed";
      break;
    case HXB_ERR_DATATYPE:
      std::cout << "Datatype mismatch";
      break;
    case HXB_ERR_INVALID_VALUE:
      std::cout << "Invalid value";
      break;
    default:
      std::cout << "(unknown)";
      break;
  }
  std::cout << std::endl;
  std::cout << std::endl;
}

void SolarCalculator::visit(const hexabus::QueryPacket& query) {}
void SolarCalculator::visit(const hexabus::EndpointQueryPacket& endpointQuery) {}

void SolarCalculator::visit(const hexabus::EndpointInfoPacket& endpointInfo)
{
  if (endpointInfo.eid() == 0) {
    std::cout << "Device Info" << std::endl
      << "Device Name:\t" << endpointInfo.value() << std::endl;
  } else {
    std::cout << "Endpoint Info\n"
      << "Endpoint ID:\t" << endpointInfo.eid() << std::endl
      << "EP Datatype:\t";
    switch(endpointInfo.datatype()) {
      case HXB_DTYPE_BOOL:
        std::cout << "Bool";
        break;
      case HXB_DTYPE_UINT8:
        std::cout << "UInt8";
        break;
      case HXB_DTYPE_UINT32:
        std::cout << "UInt32";
        break;
      case HXB_DTYPE_DATETIME:
        std::cout << "Datetime";
        break;
      case HXB_DTYPE_FLOAT:
        std::cout << "Float";
        break;
      case HXB_DTYPE_TIMESTAMP:
        std::cout << "Timestamp";
        break;
      case HXB_DTYPE_128STRING:
        std::cout << "String";
        break;
      case HXB_DTYPE_16BYTES:
        std::cout << "Binary (16bytes)";
        break;
      case HXB_DTYPE_66BYTES:
        std::cout << "Binary (66bytes)";
        break;
      default:
        std::cout << "(unknown)";
        break;
    }
    std::cout << std::endl;
    std::cout << "EP Name:\t" << endpointInfo.value() << std::endl;
  }
  std::cout << std::endl;
}


