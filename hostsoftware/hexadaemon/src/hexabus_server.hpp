#ifndef _HEXABUS_SERVER_HPP
#define _HEXABUS_SERVER_HPP

#include <boost/asio/io_service.hpp>

#include <libhexabus/device.hpp>

namespace hexadaemon {
	class HexabusServer {
		public:
			typedef boost::shared_ptr<HexabusServer> Ptr;
			HexabusServer(boost::asio::io_service& io, const std::vector<std::string>& interfaces, const std::vector<std::string>& addresses, int interval = 60, bool debug = false);
			virtual ~HexabusServer() {};

			uint32_t get_sensor(int map_idx);
			uint32_t get_sum();
			void epqueryhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid0handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid32handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void eid2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void smcontrolhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);
			void smuploadhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from);

			void broadcast_handler(const boost::system::error_code& error);

			void errorhandler(const hexabus::GenericException& error);

			void loadSensorMapping();
			std::string loadDeviceName();
			void saveDeviceName(const std::string& name);

		private:
			void _init();

		private:
			hexabus::Device _device;
			bool _debug;
			std::map<std::string, uint32_t> _flukso_values;
			std::map<int, std::string> _sensor_mapping;

			int getFluksoValue();
			void updateFluksoValues();
	};
}

#endif // _HEXABUS_SERVER_HPP
