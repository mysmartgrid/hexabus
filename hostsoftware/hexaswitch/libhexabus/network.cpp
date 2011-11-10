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
  data = NULL;
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
  char recv_data[128];
  boost::asio::ip::udp::endpoint remote_endpoint;
  my_socket->receive_from(boost::asio::buffer(recv_data, 127), remote_endpoint);
  sourceIP = remote_endpoint.address().to_string();

  if(data == NULL)
    data = (char*)malloc(128);
  memcpy(data, recv_data, 128);

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

void NetworkAccess::openSocket() { socket->open(boost::asio::ip::udp::v6()); }
void NetworkAccess::closeSocket() { socket->close(); }
char* NetworkAccess::getData() { return data; }
std::string NetworkAccess::getSourceIP() { return sourceIP; }
