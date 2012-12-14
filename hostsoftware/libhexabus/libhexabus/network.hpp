#ifndef LIBHEXABUS_NETWORK_HPP
#define LIBHEXABUS_NETWORK_HPP 1

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>
#include "../../../shared/hexabus_packet.h"
#include <boost/signals2.hpp>
#include "error.hpp"

namespace hexabus {
  class NetworkAccess {
    public:
      enum InitStyle { Unreliable, Reliable };

			typedef boost::signals2::signal<
				void (const boost::asio::ip::address_v6& source,
						const std::vector<char>& data)>
				on_packet_received_t;
			typedef on_packet_received_t::slot_type on_packet_received_slot_t;

			typedef boost::signals2::signal<void (const NetworkException& error)> on_async_error_t;
			typedef on_async_error_t::slot_type on_async_error_slot_t;

    public:
      NetworkAccess(InitStyle init);
      NetworkAccess(const std::string& interface, InitStyle init);
      NetworkAccess(const boost::asio::ip::address_v6& addr, InitStyle init);
      NetworkAccess(const boost::asio::ip::address_v6& addr, const std::string& interface, InitStyle init);
      ~NetworkAccess();
			void run();
			void stop();
			boost::signals2::connection onPacketReceived(on_packet_received_slot_t callback);
			boost::signals2::connection onAsyncError(on_async_error_slot_t callback);
      void sendPacket(std::string addr, uint16_t port, const char* data, unsigned int length);
    private:
      boost::asio::io_service io_service;
      boost::asio::ip::udp::socket socket;
			boost::asio::ip::udp::endpoint remoteEndpoint;
			on_packet_received_t packetReceived;
			on_async_error_t asyncError;
      char data[HXB_MAX_PACKET_SIZE];

      void openSocket(const boost::asio::ip::address_v6& addr, const std::string* interface, InitStyle init);

			void beginReceive();
			void packetReceiveHandler(const boost::system::error_code& error, size_t size);
  };
};

#endif
