#include "network.hpp"

#include "common.hpp"
#include "packet.hpp"
#include "crc.hpp"

#include <iostream>

using namespace hexabus;

NetworkAccess::NetworkAccess() {
  // open a socket
  io_service = new boost::asio::io_service();
  socket = new boost::asio::ip::udp::socket(*io_service);
  openSocket();
}

NetworkAccess::~NetworkAccess() {
  delete socket;
  delete io_service;
}

void NetworkAccess::receivePacket(bool related) {
  hexabus::CRC::Ptr crc(new hexabus::CRC());
  boost::asio::ip::udp::socket* my_socket;
  if(!related)
  {
    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v6(), HXB_PORT);
    my_socket = new boost::asio::ip::udp::socket(*io_service, endpoint);
  } else {
    my_socket = socket;
  }
  std::cout << "waiting for data...\n";
  char recv_data[128];
  boost::asio::ip::udp::endpoint remote_endpoint;
  my_socket->receive_from(boost::asio::buffer(recv_data, 127), remote_endpoint);
  std::cout << "recieved message from " << remote_endpoint.address().to_string() << std::endl;

  hexabus::PacketHandling phandling(recv_data);

  std::cout << "Hexabus Packet:\t" << (phandling.getOkay() ? "Yes" : "No") << "\nCRC Okay:\t" << (phandling.getCRCOkay() ? "Yes\n" : "No\n");
  if(phandling.getPacketType() == HXB_PTYPE_ERROR)
  {
    std::cout << "Packet Type:\tError\nError Code:\t";
    switch(phandling.getErrorcode())
    {
      case HXB_ERR_UNKNOWNEID:
        std::cout << "Unknown EID\n";
        break;
      case HXB_ERR_WRITEREADONLY:
        std::cout << "Write for ReadOnly Endpoint\n";
        break;
      case HXB_ERR_CRCFAILED:
        std::cout << "CRC Failed\n";
        break;
      case HXB_ERR_DATATYPE:
        std::cout << "Datatype Mismatch\n";
        break;
      default:
        std::cout << "(unknown)\n";
        break;
    }
  }
  else if(phandling.getPacketType() == HXB_PTYPE_INFO)
  {
    std::cout << "Datatype:\t";
    switch(phandling.getDatatype())
    {
      case HXB_DTYPE_BOOL:
        std::cout << "Bool\n";
        break;
      case HXB_DTYPE_UINT8:
        std::cout << "Uint8\n";
        break;
      case HXB_DTYPE_UINT32:
        std::cout << "Uint32\n";
        break;
      default:
        std::cout << "(unknown)";
        break;
    }
    std::cout << "Endpoint ID:\t" << (int)phandling.getEID() << "\nValue:\t\t";
    struct hxb_value value = phandling.getValue();
    switch(value.datatype)
    {
      case HXB_DTYPE_BOOL:
      case HXB_DTYPE_UINT8:
        std::cout << (int)value.int8;
        break;
      case HXB_DTYPE_UINT32:
        std::cout << value.int32;
        break;
      default:
        std::cout << "(unknown)";
        break;
    }
    std::cout << std::endl;
  }

  if(!related)
  {
    my_socket->close();
    delete my_socket;
  }
}

void NetworkAccess::sendPacket(std::string addr, uint16_t port, const char* data, unsigned int length) {
  boost::asio::ip::udp::endpoint remote_endpoint;
  remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(addr), port);
  boost::system::error_code error; // TODO error message?
  socket->send_to(boost::asio::buffer(data, length), remote_endpoint, 0, error);
}

void NetworkAccess::openSocket() {
  socket->open(boost::asio::ip::udp::v6());
}

void NetworkAccess::closeSocket() {
  socket->close();
}
