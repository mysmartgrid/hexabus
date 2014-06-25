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
	class SocketBase {
		public:
			typedef boost::function<bool (const Packet& packet, const boost::asio::ip::udp::endpoint& from)> filter_t;
			typedef boost::function<void (const Packet& packet, const boost::asio::ip::udp::endpoint& from)> on_packet_received_slot_t;

			typedef boost::signals2::signal<void (const GenericException& error)> on_async_error_t;
			typedef on_async_error_t::slot_type on_async_error_slot_t;

			static const boost::asio::ip::address_v6 GroupAddress;

		private:
			void openSocket();

		protected:
			boost::asio::io_service& io;
			boost::asio::ip::udp::socket socket;
			boost::asio::ip::udp::endpoint remoteEndpoint;
			boost::signals2::signal<void (const Packet::Ptr&, const boost::asio::ip::udp::endpoint&)> packetReceived;
			on_async_error_t asyncError;
			std::vector<char> data;

			int iface_idx(const std::string& iface);

			void beginReceive();
			void packetReceivedHandler(const boost::system::error_code& error, size_t size);

		public:
			SocketBase(boost::asio::io_service& io);

			virtual ~SocketBase();

			boost::asio::ip::udp::endpoint localEndpoint() const
			{
				return socket.local_endpoint();
			}

			boost::asio::io_service& ioService()
			{
				return io;
			}

			boost::signals2::connection onPacketReceived(
					const on_packet_received_slot_t& callback,
					const filter_t& filter = filtering::any());
			boost::signals2::connection onAsyncError(const on_async_error_slot_t& callback);

			std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint> receive(
					const filter_t& filter = filtering::any(),
					boost::posix_time::time_duration timeout = boost::date_time::pos_infin);
	};

	class Listener : public SocketBase {
		private:
			void configureSocket();

		public:
			Listener(boost::asio::io_service& io)
				: SocketBase(io)
			{
				configureSocket();
			}

			void listen(const std::string& dev = "");
			void ignore(const std::string& dev = "");
	};

	class Socket : public SocketBase {
		private:
			void configureSocket();

		public:
			Socket(boost::asio::io_service& io)
				: SocketBase(io)
			{
				configureSocket();
			}

			void mcast_from(const std::string& dev);

			void bind(const boost::asio::ip::address_v6& addr)
			{
				bind(boost::asio::ip::udp::endpoint(addr, 0));
			}
			void bind(const boost::asio::ip::udp::endpoint& ep);

			void send(const Packet& packet, const boost::asio::ip::address_v6& dest = GroupAddress)
			{
				send(packet, boost::asio::ip::udp::endpoint(dest, 61616));
			}
			void send(const Packet& packet, const boost::asio::ip::udp::endpoint& dest);
	};
}

#endif
