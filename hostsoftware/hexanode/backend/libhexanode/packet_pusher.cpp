#include "packet_pusher.hpp"
#include <libhexabus/error.hpp>
#include <libhexabus/crc.hpp>

using namespace hexanode;


void PacketPusher::printValueHeader(uint32_t eid, const char* datatypeStr)
{
  target << "Info" << std::endl
    << "Endpoint ID:\t" << eid << std::endl
    << "Datatype:\t" << datatypeStr << std::endl;
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


