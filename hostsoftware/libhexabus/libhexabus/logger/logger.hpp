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

class Logger : private PacketVisitor {
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
		virtual std::string get_sensor_id(const boost::asio::ip::address_v6& source, uint32_t eid);

		void on_sensor_name_received(const std::string& sensor_name, const boost::asio::ip::address_v6& address, const hexabus::Packet& ep_info);

		void on_sensor_error(const std::string& sensor_name, const hexabus::GenericException& err);

		void accept_packet(double value, uint32_t eid);

		virtual void visit(const hexabus::InfoPacket<bool>& info) { accept_packet(info.value(), info.eid()); }
		virtual void visit(const hexabus::InfoPacket<uint8_t>& info) { accept_packet(info.value(), info.eid()); }
		virtual void visit(const hexabus::InfoPacket<uint16_t>& info) { accept_packet(info.value(), info.eid()); }
		virtual void visit(const hexabus::InfoPacket<uint32_t>& info) { accept_packet(info.value(), info.eid()); }
		virtual void visit(const hexabus::InfoPacket<uint64_t>& info) { accept_packet(info.value(), info.eid()); }
		virtual void visit(const hexabus::InfoPacket<int8_t>& info) { accept_packet(info.value(), info.eid()); }
		virtual void visit(const hexabus::InfoPacket<int16_t>& info) { accept_packet(info.value(), info.eid()); }
		virtual void visit(const hexabus::InfoPacket<int32_t>& info) { accept_packet(info.value(), info.eid()); }
		virtual void visit(const hexabus::InfoPacket<int64_t>& info) { accept_packet(info.value(), info.eid()); }
		virtual void visit(const hexabus::InfoPacket<float>& info) { accept_packet(info.value(), info.eid()); }

		// FIXME: handle these properly, we should not drop valid info packets
		virtual void visit(const hexabus::InfoPacket<std::string>& info) {}
		virtual void visit(const hexabus::InfoPacket<boost::array<char, 16> >& info) {}
		virtual void visit(const hexabus::InfoPacket<boost::array<char, 65> >& info) {}

		virtual void visit(const hexabus::ErrorPacket& error) {}
		virtual void visit(const hexabus::QueryPacket& query) {}
		virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) {}
		virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo) {}

		virtual void visit(const hexabus::WritePacket<bool>& info) {}
		virtual void visit(const hexabus::WritePacket<uint8_t>& info) {}
		virtual void visit(const hexabus::WritePacket<uint16_t>& info) {}
		virtual void visit(const hexabus::WritePacket<uint32_t>& info) {}
		virtual void visit(const hexabus::WritePacket<uint64_t>& info) {}
		virtual void visit(const hexabus::WritePacket<int8_t>& info) {}
		virtual void visit(const hexabus::WritePacket<int16_t>& info) {}
		virtual void visit(const hexabus::WritePacket<int32_t>& info) {}
		virtual void visit(const hexabus::WritePacket<int64_t>& info) {}
		virtual void visit(const hexabus::WritePacket<float>& info) {}
		virtual void visit(const hexabus::WritePacket<std::string>& info) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, 16> >& info) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, 65> >& info) {}

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
