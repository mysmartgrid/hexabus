#include "logger.hpp"

#include "../../../shared/endpoints.h"

using namespace hexabus;

void Logger::operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& from)
{
	source = from.address().to_v6();
	visitPacket(packet);
}

void Logger::visit(const hexabus::InfoPacket<bool>& info)
{
	accept_packet(info.value(), info.eid());
}

void Logger::visit(const hexabus::InfoPacket<uint8_t>& info)
{
	accept_packet(info.value(), info.eid());
}

void Logger::visit(const hexabus::InfoPacket<uint32_t>& info)
{
	accept_packet(info.value(), info.eid());
}

void Logger::visit(const hexabus::InfoPacket<float>& info)
{
	accept_packet(info.value(), info.eid());
}

void Logger::visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info)
{
	accept_packet(info.value().total_seconds(), info.eid());
}

const char* Logger::eid_to_unit(uint32_t eid)
{
	hexabus::EndpointRegistry::const_iterator it;

	it = registry.find(eid);
	if (it == registry.end() || !it->second.unit()) {
		return "unknown";
	} else {
		return it->second.unit()->c_str();
	}
}

void Logger::on_sensor_name_received(const std::string& sensor_name, const hexabus::Packet& ep_info)
{
	klio::Sensor::Ptr sensor = sensor_factory.createSensor(
			static_cast<const hexabus::EndpointInfoPacket&>(ep_info).value(),
			sensor_name,
			eid_to_unit(new_sensor_backlog[sensor_name].first),
			sensor_timezone);

	new_sensor_found(sensor);
	sensor_cache.insert(std::make_pair(sensor_name, sensor));

	new_sensor_t& backlog = new_sensor_backlog[sensor_name];
	klio::readings_it_t it, end;
	for (it = backlog.second.begin(), end = backlog.second.end(); it != end; ++it) {
		record_reading(sensor, it->first, it->second);
	}

	new_sensor_backlog.erase(sensor_name);
}

void Logger::on_sensor_error(const std::string& sensor_name, const hexabus::GenericException& err)
{
	std::cerr
		<< "Error getting device name: " << err.what() << ", "
		<< "dropping " << new_sensor_backlog[sensor_name].second.size() 
		<< " readings from " << sensor_name << std::endl;

	new_sensor_backlog.erase(sensor_name);
}

void Logger::accept_packet(double value, uint32_t eid)
{
	/**
	 * 1. Create unique ID for each info message and sensor,
	 * <ip>-<endpoint>
	 */
	std::ostringstream oss;
	oss << source.to_string() << "-" << eid;
	std::string sensor_name(oss.str());

	/**
	 * 2. Ask for a sensor instance. If none is known for this
	 * sensor, create a new one.
	 */

	klio::Sensor::Ptr sensor;
	klio::timestamp_t now = tc.get_timestamp();

	if (sensor_cache.count(sensor_name)) {
		sensor = sensor_cache[sensor_name];
	} else {
		sensor = lookup_sensor(sensor_name);
	}

	if (sensor) {
		/**
		 * 3. Use the sensor instance to save the value.
		 */
		record_reading(sensor, now, value);
	} else {
		if (!new_sensor_backlog.count(sensor_name)) {
			new_sensor_backlog[sensor_name].first = eid;
			interrogator.send_request(
					source,
					hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR),
					hexabus::filtering::IsEndpointInfo(),
					boost::bind(&Logger::on_sensor_name_received, this, sensor_name, _1),
					boost::bind(&Logger::on_sensor_error, this, sensor_name, _1));
		}
		new_sensor_backlog[sensor_name].second.insert(std::make_pair(now, value));
	}
}
