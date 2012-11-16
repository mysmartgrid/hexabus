#include "network.hpp"

#include "common.hpp"
#include "packet.hpp"
#include "crc.hpp"

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>


using namespace hexabus;
NetworkAccess::NetworkAccess(const std::string& interface) :
  io_service(),
  socket(io_service)
{
  openSocket(&interface);
}

NetworkAccess::NetworkAccess() :
  io_service(),
  socket(io_service)
{
  openSocket(NULL);
}

NetworkAccess::~NetworkAccess()
{
}

void NetworkAccess::receivePacket(bool related) {
  hexabus::CRC crc;

  while (true) {
    boost::asio::ip::udp::endpoint remote_endpoint;
    socket.receive_from(boost::asio::buffer(data, sizeof(data)), remote_endpoint);
    if (!related || (related && remote_endpoint.address() == targetIP)) {
      sourceIP = remote_endpoint.address();
      return;
    }
  }
}

void NetworkAccess::sendPacket(std::string addr, uint16_t port, const char* data, unsigned int length) {
  boost::asio::ip::udp::endpoint remote_endpoint;

  targetIP = boost::asio::ip::address::from_string(addr);
  remote_endpoint = boost::asio::ip::udp::endpoint(targetIP, port);
  boost::system::error_code error; // TODO error message?
  socket.send_to(boost::asio::buffer(data, length), remote_endpoint, 0, error);
}

void NetworkAccess::openSocket(const std::string* interface) {
  socket.open(boost::asio::ip::udp::v6());
  socket.bind(
    boost::asio::ip::udp::endpoint(
      boost::asio::ip::address_v6::any(),
      HXB_PORT));

  socket.set_option(boost::asio::ip::multicast::hops(64));

  int if_index = 0;

  if (interface) {
    if_index = if_nametoindex(interface->c_str());
    if (if_index == 0) {
      // interface does not exist
      // TODO: throw som error?
      std::cerr << "Interface " << interface << " does not exist, not binding" << std::endl;
    }
    socket.set_option(boost::asio::ip::multicast::outbound_interface(if_index));
  }

  socket.set_option(
    boost::asio::ip::multicast::join_group(
      boost::asio::ip::address_v6::from_string(HXB_GROUP),
      if_index));
}



void NetworkAccess::closeSocket() { socket.close(); }
char* NetworkAccess::getData() { return data; }
boost::asio::ip::address NetworkAccess::getSourceIP() { return sourceIP; }
