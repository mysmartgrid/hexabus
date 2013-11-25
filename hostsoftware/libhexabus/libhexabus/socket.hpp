#ifndef LIBHEXABUS_SOCKET_HPP
#define LIBHEXABUS_SOCKET_HPP 1

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <string>
#include <boost/signals2.hpp>
#include <boost/date_time.hpp>
#include "error.hpp"
#include "packet.hpp"
#include "filtering.hpp"

namespace hexabus {
  class Socket {
    public:
			typedef boost::function<bool (const Packet& packet, const boost::asio::ip::udp::endpoint& from)> filter_t;
			typedef boost::function<void (const Packet& packet, const boost::asio::ip::udp::endpoint& from)> on_packet_received_slot_t;

			typedef boost::signals2::signal<void (const GenericException& error)> on_async_error_t;
			typedef on_async_error_t::slot_type on_async_error_slot_t;

    public:
			static const boost::asio::ip::address_v6 GroupAddress;

      Socket(boost::asio::io_service& io);
      Socket(boost::asio::io_service& io, const std::string& interface);
      ~Socket();

			boost::signals2::connection onPacketReceived(const on_packet_received_slot_t& callback, const filter_t& filter = filtering::any());
			boost::signals2::connection onAsyncError(const on_async_error_slot_t& callback);
      
			void listen();
			void bind(const boost::asio::ip::udp::endpoint& ep);
			void bind(const boost::asio::ip::address_v6& addr) { bind(boost::asio::ip::udp::endpoint(addr, 0)); }

			boost::asio::ip::udp::endpoint localEndpoint() const { return socket_u.local_endpoint(); }

			uint16_t send(const Packet& packet, const boost::asio::ip::udp::endpoint& dest);
			std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint> receive(const filter_t& filter = filtering::any(),
					boost::posix_time::time_duration timeout = boost::date_time::pos_infin);

			uint16_t send(const Packet& packet) { return send(packet, GroupAddress); }
			uint16_t send(const Packet& packet, const boost::asio::ip::address_v6& dest) { return send(packet, boost::asio::ip::udp::endpoint(dest, HXB_PORT)); }

			boost::asio::io_service& ioService() { return io_service; }

    private:
			struct Association {
				boost::posix_time::ptime lastUpdate;
				uint16_t seqNum;
			};

			boost::asio::io_service& io_service;
			boost::asio::ip::udp::socket socket_m, socket_u;
			boost::asio::ip::udp::endpoint remote_m, remote_u;
			boost::signals2::signal<void (const Packet::Ptr&,
					const boost::asio::ip::udp::endpoint&)> packetReceived;
			on_async_error_t asyncError;
			int if_index;
			std::vector<char> data_m, data_u;

			std::map<boost::asio::ip::udp::endpoint, Association> _associations;
			boost::asio::deadline_timer _association_gc_timer;

			void openSocket(const std::string* interface);

			void beginReceivePacket();
			void packetReceivedHandler(const boost::system::error_code& error, const std::vector<char>& buffer, const boost::asio::ip::udp::endpoint& remote, size_t size);

			void syncPacketReceiveCallback(const Packet::Ptr& packet, const boost::asio::ip::udp::endpoint& from,
					std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint>& result, const filter_t& filter);

			void associationGCTimeout(const boost::system::error_code& error);
			void scheduleAssociationGC();

			uint16_t generateSequenceNumber(const boost::asio::ip::udp::endpoint& target);
  };
};

#endif
