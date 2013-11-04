#ifndef _HEXABUS_SERVER_HPP
#define _HEXABUS_SERVER_HPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>

//#include <libhexabus/error.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>

namespace hexadaemon {
	class HexabusServer {
		public:
			typedef boost::shared_ptr<HexabusServer> Ptr;
			HexabusServer(boost::asio::io_service& io, int interval = 60, bool debug = false);
			virtual ~HexabusServer() {};

			void epqueryhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid0handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid32handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void l1handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void l2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void l3handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void s01handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void s02handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);

			void broadcast_handler(const boost::system::error_code& error);

			void errorhandler(const hexabus::GenericException& error);

			void loadSensorMapping();

		private:
			hexabus::Socket _socket;
			boost::asio::deadline_timer _timer;
			int _interval;
			bool _debug;
			std::map<std::string, uint32_t> _flukso_values;
			std::map<int, std::string> _sensor_mapping;

			int getFluksoValue();
			void updateFluksoValues();
	};
}

#endif // _HEXABUS_SERVER_HPP
