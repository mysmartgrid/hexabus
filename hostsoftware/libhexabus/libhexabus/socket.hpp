#ifndef LIBHEXABUS_SOCKET_HPP
#define LIBHEXABUS_SOCKET_HPP 1

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>
#include "../../../shared/hexabus_packet.h"
#include <boost/signals2.hpp>
#include "error.hpp"
#include "packet.hpp"

namespace hexabus {
  class Socket {
    public:
      enum InitStyle { Unreliable, Reliable };

			typedef boost::signals2::signal<
				void (const boost::asio::ip::address_v6& source,
						const Packet& packet)>
				on_packet_received_t;
			typedef on_packet_received_t::slot_type on_packet_received_slot_t;

			typedef boost::signals2::signal<void (const GenericException& error)> on_async_error_t;
			typedef on_async_error_t::slot_type on_async_error_slot_t;

    public:
      Socket(InitStyle init);
      Socket(const std::string& interface, InitStyle init);
      Socket(const boost::asio::ip::address_v6& addr, InitStyle init);
      Socket(const boost::asio::ip::address_v6& addr, const std::string& interface, InitStyle init);
      ~Socket();
			void run();
			void stop();
			boost::signals2::connection onPacketReceived(on_packet_received_slot_t callback);
			boost::signals2::connection onAsyncError(on_async_error_slot_t callback);
      void sendPacket(std::string addr, uint16_t port, const Packet& packet);
    private:
      boost::asio::io_service io_service;
      boost::asio::ip::udp::socket socket;
			boost::asio::ip::udp::endpoint remoteEndpoint;
			on_packet_received_t packetReceived;
			on_async_error_t asyncError;
			std::vector<char> data;

      void openSocket(const boost::asio::ip::address_v6& addr, const std::string* interface, InitStyle init);

			void beginReceive();
			void packetReceiveHandler(const boost::system::error_code& error, size_t size);
  };
};

#endif
