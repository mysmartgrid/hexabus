#ifndef LIBHEXALOG_LOGGER_HPP
#define LIBHEXALOG_LOGGER_HPP 1

#include <string>
#include <map>

#include <libklio/sensor.hpp>
#include <libklio/time.hpp>
#include <libklio/sensor-factory.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/endpoint_registry.hpp>
#include <libhexabus/device_interrogator.hpp>

namespace hexabus {

class Logger :
		private PacketVisitor,
		virtual hexabus::TypedPacketVisitor<hexabus::ReportPacket>::Empty, 
		virtual hexabus::TypedPacketVisitor<hexabus::ProxyInfoPacket>::Empty {
	protected:
		klio::TimeConverter& tc;
		klio::SensorFactory& sensor_factory;
		std::string sensor_timezone;
		hexabus::DeviceInterrogator& interrogator;

		hexabus::EndpointRegistry& registry;

		typedef std::pair<uint32_t, klio::readings_t> new_sensor_t;
		std::map<std::string, new_sensor_t> new_sensor_backlog;
		std::map<std::string, klio::Sensor::Ptr> sensor_cache;

		boost::asio::ip::address_v6 source;

		virtual const char* eid_to_unit(uint32_t eid);

		void on_sensor_name_received(const std::string& sensor_name, const boost::asio::ip::address_v6& address, const hexabus::Packet& ep_info);

		void on_sensor_error(const std::string& sensor_name, const hexabus::GenericException& err);

		void accept_packet(double value, uint32_t eid);

		virtual void visit(const hexabus::InfoPacket<bool>& info);
		virtual void visit(const hexabus::InfoPacket<uint8_t>& info);
		virtual void visit(const hexabus::InfoPacket<uint32_t>& info);
		virtual void visit(const hexabus::InfoPacket<float>& info);
		virtual void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info);

		// FIXME: handle these properly, we should not drop valid info packets
		// REPORT and PINFO packets are also dropped, as indicated by the ::Empty subvisitors
		virtual void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info) {} 
		virtual void visit(const hexabus::InfoPacket<std::string>& info) {} 
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {} 
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {}

		virtual void visit(const hexabus::ErrorPacket& error) {}
		virtual void visit(const hexabus::QueryPacket& query) {}
		virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) {}
		virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo) {}
		virtual void visit(const hexabus::EndpointReportPacket& endpointInfo) {}
		virtual void visit(const hexabus::AckPacket& ack) {}

		virtual void visit(const hexabus::WritePacket<bool>& write) {}
		virtual void visit(const hexabus::WritePacket<uint8_t>& write) {}
		virtual void visit(const hexabus::WritePacket<uint32_t>& write) {}
		virtual void visit(const hexabus::WritePacket<float>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) {}
		virtual void visit(const hexabus::WritePacket<std::string>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}

	protected:
		virtual void record_reading(klio::Sensor::Ptr sensor, klio::timestamp_t ts, double value) = 0;
		virtual void new_sensor_found(klio::Sensor::Ptr sensor, const boost::asio::ip::address_v6& address) = 0;
		virtual klio::Sensor::Ptr lookup_sensor(const std::string& name) = 0;

	public:
		Logger(klio::TimeConverter& tc,
			klio::SensorFactory& sensor_factory,
			const std::string& sensor_timezone,
			hexabus::DeviceInterrogator& interrogator,
			hexabus::EndpointRegistry& registry)
			: tc(tc), sensor_factory(sensor_factory), sensor_timezone(sensor_timezone), interrogator(interrogator), registry(registry)
		{
		}

		virtual ~Logger()
		{
		}

		void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& from);
};

}

#endif
