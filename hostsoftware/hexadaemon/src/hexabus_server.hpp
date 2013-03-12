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
			HexabusServer(boost::asio::io_service& io, int interval = 60);
			virtual ~HexabusServer() {};

			void epqueryhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid0handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid21handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);

			void broadcast_handler(const boost::system::error_code& error);

			void errorhandler(const hexabus::GenericException& error);

		private:
			hexabus::Socket _socket;
			boost::asio::deadline_timer _timer;
			int _interval;
			std::map<std::string, uint16_t> _flukso_values;

			int getFluksoValue();
			void updateFluksoValues();
	};
}

#endif // _HEXABUS_SERVER_HPP
