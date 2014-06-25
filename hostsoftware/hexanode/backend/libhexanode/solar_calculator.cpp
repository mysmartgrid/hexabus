#include "solar_calculator.hpp"
#include <libhexabus/packet.hpp>
#include <numeric>
#include "../../../../shared/endpoints.h"
#include <cstdlib>

using namespace hexanode;

void SolarCalculator::push_value(const uint32_t eid, 
    const uint8_t value)
{
  std::cout << "Received EID " << eid 
    << " value " << (uint16_t) value << std::endl;
  if (eid == _pv_production_eid) {
    uint32_t current_production = value * (_pv_peak_watt/255);
    _historian->add_production(_endpoint, eid, current_production);
    std::cout << "Sending calculated production: " << current_production 
      << std::endl;
    _send_socket->send(
        hexabus::InfoPacket<uint32_t>(EP_PV_PRODUCTION, current_production));
  } else if (eid == _battery_eid) {
    int32_t current_battery = ((value-127) * (_battery_peak_watt/127));
    if (abs(current_battery) < 50) {
      current_battery = 0;
    }
    _historian->remove_device(_endpoint, eid);
    if (current_battery < 0) {
      _historian->add_production(_endpoint, eid, -1*current_battery);
    } else {
      _historian->add_consumption(_endpoint, eid, current_battery);
    }
    std::cout << "Battery update: " << current_battery << std::endl;
    _send_socket->send(
        hexabus::InfoPacket<float>(EP_BATTERY_BALANCE, current_battery));
  } else {
    std::cout << "Received unknown EID - discarding." << std::endl;
  }
}


void SolarCalculator::push_value(const uint32_t eid, 
    const uint32_t value)
{
  std::cout << "Received EID " << eid
    << " value " << value << std::endl;
  switch (eid) {
    case EP_POWER_METER:
      {
        std::cout << "Received power measurement of " << value << std::endl;
        _historian->add_consumption(_endpoint, eid, value);
        break;
      }
    default:
      std::cout << "Received unknown EID - discarding." << std::endl;
  }
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

void SolarCalculator::printValuePacket(
    const hexabus::ValuePacket<uint32_t>& packet, const char* datatypeStr)
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
      case HXB_DTYPE_65BYTES:
        std::cout << "Binary (65bytes)";
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


