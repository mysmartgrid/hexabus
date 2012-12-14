#ifndef LIBHEXABUS_NETWORK_HPP
#define LIBHEXABUS_NETWORK_HPP 1

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>
#include "../../../shared/hexabus_packet.h"

namespace hexabus {
  class NetworkAccess {
    public:
      enum InitStyle { Unreliable, Reliable };
    public:
      NetworkAccess(InitStyle init);
      NetworkAccess(const std::string& interface, InitStyle init);
      NetworkAccess(const boost::asio::ip::address_v6& addr, InitStyle init);
      NetworkAccess(const boost::asio::ip::address_v6& addr, const std::string& interface, InitStyle init);
      ~NetworkAccess();
      void receivePacket(bool related); // related==true: receive data from the address where the last packet was sent to
      void sendPacket(std::string addr, uint16_t port, const char* data, unsigned int length);
      boost::asio::ip::address getSourceIP();
      char* getData();
    private:
      void openSocket(const boost::asio::ip::address_v6& addr, const std::string* interface, InitStyle init);
      boost::asio::io_service io_service;
      boost::asio::ip::udp::socket socket;
      boost::asio::ip::address sourceIP;
      boost::asio::ip::address targetIP;
      char data[HXB_MAX_PACKET_SIZE];
  };
};

#endif
