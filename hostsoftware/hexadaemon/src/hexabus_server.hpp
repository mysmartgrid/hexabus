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
			HexabusServer(boost::asio::io_service& io, const std::string& interface, const std::string& address, int interval = 60, bool debug = false);
			virtual ~HexabusServer() {};

			void epqueryhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid0handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid32handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void smcontrolhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void smuploadhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);

			void value_handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from, int map_idx);

			void broadcast_handler(const boost::system::error_code& error);

			void errorhandler(const hexabus::GenericException& error);

			void loadSensorMapping();
			std::string loadDeviceName();
			void saveDeviceName(std::string name);

		private:
			void _init();

		private:
			hexabus::Listener _listener;
			hexabus::Socket _socket;
			boost::asio::deadline_timer _timer;
			int _interval;
			bool _debug;
			uint8_t _sm_state;
			std::string _device_name;
			std::map<std::string, uint32_t> _flukso_values;
			std::map<int, std::string> _sensor_mapping;

			int getFluksoValue();
			void updateFluksoValues();
	};
}

#endif // _HEXABUS_SERVER_HPP
