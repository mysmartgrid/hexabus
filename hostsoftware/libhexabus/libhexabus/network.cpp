#include "network.hpp"

#include "common.hpp"
#include "packet.hpp"
#include "crc.hpp"
#include "error.hpp"

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>


using namespace hexabus;
NetworkAccess::NetworkAccess(const std::string& interface, InitStyle init) :
  io_service(),
  socket(io_service)
{
  openSocket(&interface, init);
}

NetworkAccess::NetworkAccess(InitStyle init) :
  io_service(),
  socket(io_service)
{
  openSocket(NULL, init);
}

NetworkAccess::~NetworkAccess()
{
}

void NetworkAccess::receivePacket(bool related) {
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
  
  boost::system::error_code error;
  socket.send_to(boost::asio::buffer(data, length), remote_endpoint, 0, error);

  if (error) {
    throw NetworkException(addr);
  }
}

void NetworkAccess::openSocket(const std::string* interface, InitStyle init) {
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
      throw NetworkException(*interface);
    }
    socket.set_option(boost::asio::ip::multicast::outbound_interface(if_index));
  }

  socket.set_option(
    boost::asio::ip::multicast::join_group(
      boost::asio::ip::address_v6::from_string(HXB_GROUP),
      if_index));

  if (init == Reliable) {
    // Send invalid packets and wait for a second to build state on multicast routers on the way
    // Two packets will be sent, with one second delay after each. This ensures that routing state is built
    // on at least two consecutive hops, which should be enough for time being.
    // Ideally, this should be replaced by something less reminiscent of brute force
    for (int i = 0; i < 2; i++) {
      sendPacket(HXB_GROUP, 61616, 0, 0);

      timeval timeout = { 1, 0 };
      select(0, 0, 0, 0, &timeout);
    }
  }
}



void NetworkAccess::closeSocket() { socket.close(); }
char* NetworkAccess::getData() { return data; }
boost::asio::ip::address NetworkAccess::getSourceIP() { return sourceIP; }
