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
  io_service(new boost::asio::io_service()),
  socket(new boost::asio::ip::udp::socket(*io_service)),
  data(NULL)
{ 
  openSocket(interface);
}

NetworkAccess::NetworkAccess() : 
  io_service(new boost::asio::io_service()),
  socket(new boost::asio::ip::udp::socket(*io_service)),
  data(NULL)
{
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
	my_socket->set_option(boost::asio::ip::multicast::join_group(
				boost::asio::ip::address::from_string(HXB_GROUP)));
  } else {
    my_socket = socket;
  }
  char recv_data[256];
  boost::asio::ip::udp::endpoint remote_endpoint;
  my_socket->receive_from(boost::asio::buffer(recv_data, 256), remote_endpoint);
  sourceIP = remote_endpoint.address();

  if(data == NULL)
    data = (char*)malloc(256);
  memcpy(data, recv_data, 256);

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
  socket->send_to(boost::asio::buffer(data, length), remote_endpoint, 0, error);
}

void NetworkAccess::openSocket() { 
  socket->open(boost::asio::ip::udp::v6()); 
}

// TODO: Proper error handling.
void NetworkAccess::openSocket(const std::string& interface) { 
  socket->open(boost::asio::ip::udp::v6()); 
#ifdef HAS_LINUX
  int native_sock = socket->native();
  int result = -1;
  if (native_sock != -1) {
    result = setsockopt(native_sock, SOL_SOCKET, SO_BINDTODEVICE, 
        interface.c_str(), sizeof(interface.c_str()));
    if (result) {
      std::cerr << "Cannot use interface " << interface << ": ";
      std::cerr << " " << strerror(errno) << std::endl;
    }
  } else {
    std::cerr << "Cannot use interface " << interface 
      << ", no native socket aquired: ";
    std::cerr << strerror(errno) << std::endl;
  }
#endif // HAS_LINUX
#ifdef MAC_OS
  std::cout << "Currently no support for setting the device on Mac OS." << std::endl;
#endif // MAC_OS
}


void NetworkAccess::closeSocket() { socket->close(); }
char* NetworkAccess::getData() { return data; }
boost::asio::ip::address NetworkAccess::getSourceIP() { return sourceIP; }
