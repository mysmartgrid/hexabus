#ifndef _HEXABUS_SERVER_HPP
#define _HEXABUS_SERVER_HPP

#include <boost/asio/io_service.hpp>

#include <libhexabus/socket.hpp>
#include <libhexabus/packet.hpp>

namespace hexadaemon {
	class HexabusServer {
		public:
			typedef boost::shared_ptr<HexabusServer> Ptr;
			HexabusServer(boost::asio::io_service& io);
			virtual ~HexabusServer() {};

			void epqueryhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid0handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);

		private:
			hexabus::Socket _socket;
	};
}

#endif // _HEXABUS_SERVER_HPP
