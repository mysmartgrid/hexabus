#ifndef LIBHEXABUS_NETWORK_HPP
#define LIBHEXABUS_NETWORK_HPP 1

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>
#include "../../../shared/hexabus_packet.h"

namespace hexabus {
  class NetworkAccess {
    public:
      NetworkAccess();
      ~NetworkAccess();
      void openSocket();
      void closeSocket();
      void receivePacket(bool related);
      void sendPacket(std::string addr, uint16_t port, const char* data, unsigned int lengt);
    private:
      boost::asio::io_service* io_service;
      boost::asio::ip::udp::socket* socket;
  };
};

#endif
