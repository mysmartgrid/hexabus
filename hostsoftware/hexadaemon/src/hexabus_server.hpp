#ifndef _HEXABUS_SERVER_HPP
#define _HEXABUS_SERVER_HPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>

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

			void broadcast_handler(const boost::system::error_code& error);

		private:
			hexabus::Socket _socket;
			boost::asio::deadline_timer _timer;
			std::map<std::string, uint16_t> _flukso_values;

			int getFluksoValue();
			void updateFluksoValues();
	};
}

#endif // _HEXABUS_SERVER_HPP
