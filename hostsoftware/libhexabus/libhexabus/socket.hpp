#ifndef LIBHEXABUS_SOCKET_HPP
#define LIBHEXABUS_SOCKET_HPP 1

#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>
#include <string>
#include <boost/signals2.hpp>
#include <boost/date_time.hpp>
#include <boost/shared_ptr.hpp>
#include <queue>
#include "error.hpp"
#include "packet.hpp"
#include "filtering.hpp"

#define RETRANS_TIMEOUT 1
#define RETRANS_LIMIT 3

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

			virtual bool packetPrereceive(const Packet::Ptr& packet, const boost::asio::ip::udp::endpoint& from) {
				return true;
			}

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
		public:
			typedef boost::function<void (const Packet& packet, const boost::asio::ip::udp::endpoint& from, bool transmissionFailed)> on_packet_transmitted_callback_t;
		private:
			void configureSocket();

			struct Association {
				Association(boost::asio::io_service& io) : timeout_timer(io) {}

				boost::posix_time::ptime lastUpdate;
				uint16_t seqNum;
				uint16_t rSeqNum;
				std::queue< std::pair <boost::shared_ptr<const Packet>, on_packet_transmitted_callback_t> > sendQueue;
				uint16_t retrans_count;
				uint16_t want_ack_for;
				boost::asio::deadline_timer timeout_timer;
				std::pair<boost::shared_ptr<const Packet>, on_packet_transmitted_callback_t> currentPacket;
			};

			on_packet_transmitted_callback_t transmittedPacket;
			std::map<boost::asio::ip::udp::endpoint, boost::shared_ptr<Association> > _associations;
			boost::asio::deadline_timer _association_gc_timer;
			boost::signals2::connection ack_reply;
			filter_t ackfilter;

			void associationGCTimeout(const boost::system::error_code& error);
			void scheduleAssociationGC();

			void beginSend(const boost::asio::ip::udp::endpoint& to);
			void onSendTimeout(const boost::asio::ip::udp::endpoint& to);

			uint16_t send(const Packet& packet, const boost::asio::ip::udp::endpoint& dest, uint16_t seqNum);
			bool packetPrereceive(const Packet::Ptr& packet, const boost::asio::ip::udp::endpoint& from);

		public:
			Socket(boost::asio::io_service& io)
				: SocketBase(io), _association_gc_timer(io)
			{
				configureSocket();
				filterAckReplys();
			}

			void mcast_from(const std::string& dev);

			void bind(const boost::asio::ip::address_v6& addr)
			{
				bind(boost::asio::ip::udp::endpoint(addr, 0));
			}
			void bind(const boost::asio::ip::udp::endpoint& ep);

			uint16_t send(const Packet& packet, const boost::asio::ip::address_v6& dest = GroupAddress)
			{
				return send(packet, boost::asio::ip::udp::endpoint(dest, HXB_PORT));
			}

			uint16_t send(const Packet& packet, const boost::asio::ip::udp::endpoint& dest)
			{
				return send(packet, dest, generateSequenceNumber(dest));
			}
			
			void onPacketTransmitted(
					const on_packet_transmitted_callback_t& callback, const Packet& packet,
					const boost::asio::ip::udp::endpoint& dest);

			void filterAckReplys(const filter_t& filter = filtering::any());

			uint16_t generateSequenceNumber(const boost::asio::ip::udp::endpoint& target);
			Association& getAssociation(const boost::asio::ip::udp::endpoint& target);
	};
}

#endif
