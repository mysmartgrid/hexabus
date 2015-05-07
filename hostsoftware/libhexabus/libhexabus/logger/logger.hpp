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
		virtual void visit(const hexabus::InfoPacket<std::array<uint8_t, 16> >& info) {}
		virtual void visit(const hexabus::InfoPacket<std::array<uint8_t, 65> >& info) {}

		virtual void visit(const hexabus::ErrorPacket& error) {}
		virtual void visit(const hexabus::QueryPacket& query) {}
		virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) {}
		virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo) {}
		virtual void visit(const hexabus::EndpointReportPacket& endpointReport) {}
		virtual void visit(const hexabus::AckPacket& ack) {}
		virtual void visit(const hexabus::PropertyQueryPacket& propertyQuery) {}

		virtual void visit(const hexabus::WritePacket<bool>& write) {}
		virtual void visit(const hexabus::WritePacket<uint8_t>& write) {}
		virtual void visit(const hexabus::WritePacket<uint16_t>& write) {}
		virtual void visit(const hexabus::WritePacket<uint32_t>& write) {}
		virtual void visit(const hexabus::WritePacket<uint64_t>& write) {}
		virtual void visit(const hexabus::WritePacket<int8_t>& write) {}
		virtual void visit(const hexabus::WritePacket<int16_t>& write) {}
		virtual void visit(const hexabus::WritePacket<int32_t>& write) {}
		virtual void visit(const hexabus::WritePacket<int64_t>& write) {}
		virtual void visit(const hexabus::WritePacket<float>& write) {}
		virtual void visit(const hexabus::WritePacket<std::string>& write) {}
		virtual void visit(const hexabus::WritePacket<std::array<uint8_t, 16> >& write) {}
		virtual void visit(const hexabus::WritePacket<std::array<uint8_t, 65> >& write) {}

		virtual void visit(const hexabus::ReportPacket<bool>& report) {}
		virtual void visit(const hexabus::ReportPacket<uint8_t>& report) {}
		virtual void visit(const hexabus::ReportPacket<uint16_t>& report) {}
		virtual void visit(const hexabus::ReportPacket<uint32_t>& report) {}
		virtual void visit(const hexabus::ReportPacket<uint64_t>& report) {}
		virtual void visit(const hexabus::ReportPacket<int8_t>& report) {}
		virtual void visit(const hexabus::ReportPacket<int16_t>& report) {}
		virtual void visit(const hexabus::ReportPacket<int32_t>& report) {}
		virtual void visit(const hexabus::ReportPacket<int64_t>& report) {}
		virtual void visit(const hexabus::ReportPacket<float>& report) {}
		virtual void visit(const hexabus::ReportPacket<std::string>& report) {}
		virtual void visit(const hexabus::ReportPacket<std::array<uint8_t, 16> >& report) {}
		virtual void visit(const hexabus::ReportPacket<std::array<uint8_t, 65> >& report) {}

		virtual void visit(const hexabus::ProxyInfoPacket<bool>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<uint8_t>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<uint16_t>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<uint32_t>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<uint64_t>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<int8_t>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<int16_t>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<int32_t>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<int64_t>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<float>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<std::string>& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<std::array<uint8_t, 16> >& proxyInfo) {}
		virtual void visit(const hexabus::ProxyInfoPacket<std::array<uint8_t, 65> >& proxyInfo) {}

		virtual void visit(const hexabus::PropertyWritePacket<bool>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<uint8_t>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<uint16_t>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<uint32_t>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<uint64_t>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<int8_t>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<int16_t>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<int32_t>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<int64_t>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<float>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<std::string>& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<std::array<uint8_t, 16> >& propertyWrite) {}
		virtual void visit(const hexabus::PropertyWritePacket<std::array<uint8_t, 65> >& propertyWrite) {}

		virtual void visit(const hexabus::PropertyReportPacket<bool>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<uint8_t>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<uint16_t>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<uint32_t>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<uint64_t>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<int8_t>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<int16_t>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<int32_t>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<int64_t>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<float>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<std::string>& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<std::array<uint8_t, 16> >& propertyReport) {}
		virtual void visit(const hexabus::PropertyReportPacket<std::array<uint8_t, 65> >& propertyReport) {}

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
