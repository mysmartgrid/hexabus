#include "packet_pusher.hpp"
#include <libhexabus/error.hpp>
#include <libhexabus/crc.hpp>
#include "../../../shared/endpoints.h"

using namespace hexanode;


void PacketPusher::push_value(uint32_t eid, 
    const std::string& value)
{
  std::ostringstream oss;
  oss << _endpoint << "%" << eid;
  std::string sensor_id=oss.str();
  bool success=false;
  uint8_t retry_counter=0;
  while (! success && retry_counter < 3) {
    retry_counter++;
    try {
      std::cerr << "fooooooo" << sensor_id << std::endl;
      hexanode::Sensor::Ptr sensor=_sensors->get_by_id(sensor_id);
      sensor->post_value(_client, _api_uri, value);
      target << "Sensor " << sensor_id << ": submitted value " << value << std::endl;
      std::cerr << "asdf\n";
      success=true;
    } catch (const hexanode::NotFoundException& e) {
      // The sensor was not found during the get_by_id call.
      std::cout << "No information regarding sensor " << sensor_id 
        << " found - creating sensor with boilerplate data." << std::endl;
      int min_value = 0;
      int max_value = 0;
      switch(eid) {
        case EP_POWER_METER: min_value = 0; max_value = 3200; break;
        case EP_TEMPERATURE: min_value = 15; max_value = 30; break;
        case EP_HUMIDITY: min_value = 0; max_value = 100; break;
        case EP_PRESSURE: min_value = 900; max_value = 1050; break;
      }
      hexanode::Sensor::Ptr new_sensor(new hexanode::Sensor(sensor_id, 
            _info->get_device_name(_endpoint.address().to_v6()),
            min_value, max_value));
      _sensors->add_sensor(new_sensor);
    } catch (const hexanode::CommunicationException& e) {
      // An error occured during network communication.
      std::cout << "Cannot submit sensor values (" << e.what() << ")" << std::endl;
      std::cout << "Attempting to define sensor at the frontend cache." << std::endl;
      hexanode::Sensor::Ptr sensor=_sensors->get_by_id(sensor_id);
      sensor->put(_client, _api_uri, "n/a"); 
    } catch (const std::exception& e) {
      target << "Cannot recover from error: " << e.what() << std::endl;
    }
  }
  if (! success) {
    target << "FAILED to submit sensor value.";
  }
}


void PacketPusher::printValueHeader(uint32_t eid, const char* datatypeStr)
{
  target << " EID " << eid;
}



void PacketPusher::printValuePacket(const hexabus::ValuePacket<uint8_t>& packet, const char* datatypeStr)
{
  printValueHeader(packet.eid(), datatypeStr);
  target << "Value:\t" << (int) packet.value() << std::endl;
  target << std::endl;
}



void PacketPusher::visit(const hexabus::ErrorPacket& error)
{
  target << "Error" << std::endl
    << "Error code:\t";
  switch (error.code()) {
    case HXB_ERR_UNKNOWNEID:
      target << "Unknown EID";
      break;
    case HXB_ERR_WRITEREADONLY:
      target << "Write on readonly endpoint";
      break;
    case HXB_ERR_CRCFAILED:
      target << "CRC failed";
      break;
    case HXB_ERR_DATATYPE:
      target << "Datatype mismatch";
      break;
    case HXB_ERR_INVALID_VALUE:
      target << "Invalid value";
      break;
    default:
      target << "(unknown)";
      break;
  }
  target << std::endl;
  target << std::endl;
}

void PacketPusher::visit(const hexabus::QueryPacket& query) {}
void PacketPusher::visit(const hexabus::EndpointQueryPacket& endpointQuery) {}

void PacketPusher::visit(const hexabus::EndpointInfoPacket& endpointInfo)
{
  if (endpointInfo.eid() == 0) {
    target << "Device Info" << std::endl
      << "Device Name:\t" << endpointInfo.value() << std::endl;
  } else {
    target << "Endpoint Info\n"
      << "Endpoint ID:\t" << endpointInfo.eid() << std::endl
      << "EP Datatype:\t";
    switch(endpointInfo.datatype()) {
      case HXB_DTYPE_BOOL:
        target << "Bool";
        break;
      case HXB_DTYPE_UINT8:
        target << "UInt8";
        break;
      case HXB_DTYPE_UINT32:
        target << "UInt32";
        break;
      case HXB_DTYPE_DATETIME:
        target << "Datetime";
        break;
      case HXB_DTYPE_FLOAT:
        target << "Float";
        break;
      case HXB_DTYPE_TIMESTAMP:
        target << "Timestamp";
        break;
      case HXB_DTYPE_128STRING:
        target << "String";
        break;
      case HXB_DTYPE_16BYTES:
        target << "Binary (16bytes)";
        break;
      case HXB_DTYPE_66BYTES:
        target << "Binary (66bytes)";
        break;
      default:
        target << "(unknown)";
        break;
    }
    target << std::endl;
    target << "EP Name:\t" << endpointInfo.value() << std::endl;
  }
  target << std::endl;
}


