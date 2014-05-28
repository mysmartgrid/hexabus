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

void Logger::on_sensor_name_received(const std::string& sensor_id, const boost::asio::ip::address_v6& address, const hexabus::Packet& ep_info)
{
	klio::Sensor::Ptr sensor = sensor_factory.createSensor(
			sensor_id,
			static_cast<const hexabus::EndpointInfoPacket&>(ep_info).value(),
			eid_to_unit(new_sensor_backlog[sensor_id].first),
			sensor_timezone);

	new_sensor_found(sensor, address);
	sensor_cache.insert(std::make_pair(sensor_id, sensor));

	new_sensor_t& backlog = new_sensor_backlog[sensor_id];
	klio::readings_it_t it, end;
	for (it = backlog.second.begin(), end = backlog.second.end(); it != end; ++it) {
		record_reading(sensor, it->first, it->second);
	}

	new_sensor_backlog.erase(sensor_id);
}

void Logger::on_sensor_error(const std::string& sensor_id, const hexabus::GenericException& err)
{
	std::cerr
		<< "Error getting device name: " << err.what() << ", "
		<< "dropping " << new_sensor_backlog[sensor_id].second.size()
		<< " readings from " << sensor_id << std::endl;

	new_sensor_backlog.erase(sensor_id);
}

std::string Logger::get_sensor_id(const boost::asio::ip::address_v6& source, uint32_t eid)
{
	std::ostringstream oss;
	oss << source.to_string() << "-" << eid;
	return oss.str();
}

void Logger::accept_packet(double value, uint32_t eid)
{
	/**
	 * 1. Create unique ID for each info message and sensor,
	 * <ip>-<endpoint>
	 */
	std::string sensor_id(get_sensor_id(source, eid));

	/**
	 * 2. Ask for a sensor instance. If none is known for this
	 * sensor, create a new one.
	 */

	klio::Sensor::Ptr sensor;
	klio::timestamp_t now = tc.get_timestamp();

	if (sensor_cache.count(sensor_id)) {
		sensor = sensor_cache[sensor_id];
	} else {
		sensor = lookup_sensor(sensor_id);
	}

	if (sensor) {
		/**
		 * 3. Use the sensor instance to save the value.
		 */
		record_reading(sensor, now, value);
	} else {
		if (!new_sensor_backlog.count(sensor_id)) {
			new_sensor_backlog[sensor_id].first = eid;
			interrogator.send_request(
					source,
					hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR),
					hexabus::filtering::IsEndpointInfo(),
					boost::bind(&Logger::on_sensor_name_received, this, sensor_id, source, _1),
					boost::bind(&Logger::on_sensor_error, this, sensor_id, _1));
		}
		new_sensor_backlog[sensor_id].second.insert(std::make_pair(now, value));
	}
}
