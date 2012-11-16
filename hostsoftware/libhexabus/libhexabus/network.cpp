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
  openSocket(interface);
}

NetworkAccess::NetworkAccess() :
  io_service(),
  socket(io_service)
{
  openSocket();
}

NetworkAccess::~NetworkAccess()
{
}

void NetworkAccess::receivePacket(bool related) {
  hexabus::CRC::Ptr crc(new hexabus::CRC());
  boost::asio::ip::udp::socket* my_socket;
  if(!related)
  {
    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v6(), HXB_PORT);
    my_socket = new boost::asio::ip::udp::socket(io_service, endpoint);
	my_socket->set_option(boost::asio::ip::multicast::join_group(
				boost::asio::ip::address::from_string(HXB_GROUP)));
  } else {
    my_socket = &socket;
  }

  boost::asio::ip::udp::endpoint remote_endpoint;
  my_socket->receive_from(boost::asio::buffer(data, sizeof(data)), remote_endpoint);
  sourceIP = remote_endpoint.address();

  if(!related)
  {
    my_socket->close();
    delete my_socket;
  }
}

void NetworkAccess::sendPacket(std::string addr, uint16_t port, const char* data, unsigned int length) {
  boost::asio::ip::udp::endpoint remote_endpoint;
  remote_endpoint = boost::asio::ip::udp::endpoint(
    boost::asio::ip::address::from_string(addr), port);
  boost::system::error_code error; // TODO error message?
  socket.send_to(boost::asio::buffer(data, length), remote_endpoint, 0, error);
}

void NetworkAccess::openSocket() {
  socket.open(boost::asio::ip::udp::v6());

  socket.set_option(boost::asio::ip::multicast::hops(64));
}

// TODO: Proper error handling.
void NetworkAccess::openSocket(const std::string& interface) {
  openSocket();

  int if_index = if_nametoindex(interface.c_str());
  if (if_index == 0) {
    // interface does not exist
    // TODO: throw som error?
    std::cerr << "Interface " << interface << " does not exist, not binding" << std::endl;
  }
  socket.set_option(boost::asio::ip::multicast::outbound_interface(if_index));
}


void NetworkAccess::closeSocket() { socket.close(); }
char* NetworkAccess::getData() { return data; }
boost::asio::ip::address NetworkAccess::getSourceIP() { return sourceIP; }
