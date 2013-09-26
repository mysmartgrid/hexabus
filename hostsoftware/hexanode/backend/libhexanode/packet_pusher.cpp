#include "packet_pusher.hpp"
#include <libhexabus/error.hpp>
#include <libhexabus/crc.hpp>
#include <boost/lexical_cast.hpp>
#include "../../../shared/endpoints.h"

using namespace hexanode;

void PacketPusher::deviceInfoReceived(const boost::asio::ip::address_v6& device, const hexabus::Packet& info)
{
	if (_unidentified_devices.count(device) == 0) {
		return;
	}

	typedef std::set<uint32_t>::const_iterator iter;
	std::set<uint32_t> eids = _unidentified_devices[device];

	for (iter it = eids.begin(), end = eids.end(); it != end; ++it) {
		std::ostringstream oss;
		oss << device << "_" << *it;
		std::string sensor_id = oss.str();

		std::cout << "Creating sensor " << sensor_id << " with boilerplate data" << std::endl;
		int min_value = 0;
		int max_value = 0;
		std::string unit("");
		/*
		 * TODO: Hack for intersolar, clean things up. This should reside in 
		 * a separate configuration file (propertytree parser)
		 */
		switch(*it) {
			case EP_POWER_METER: min_value = 0; max_value = 3200; unit="W"; break;
			case EP_TEMPERATURE: min_value = 15; max_value = 30; unit="°C"; break;
			case EP_HUMIDITY: min_value = 0; max_value = 100; unit="% r.h."; break;
			case EP_PRESSURE: min_value = 900; max_value = 1050; unit="hPa"; break;
		}
		hexanode::Sensor::Ptr new_sensor(new hexanode::Sensor(sensor_id, 
					static_cast<const hexabus::EndpointInfoPacket&>(info).value() + " [" + unit + "]",
					min_value, max_value,
					std::string("sensors")
					));
		_sensors->add_sensor(new_sensor);
	}

	_unidentified_devices.erase(device);
}

void PacketPusher::deviceInfoError(const boost::asio::ip::address_v6& device, const hexabus::GenericException& error)
{
	_unidentified_devices.erase(device);
	std::cout << "No reply on device descriptor EPQuery from " << device << ": " << error.what() << std::endl;
}

void PacketPusher::push_value(uint32_t eid, 
    const std::string& value)
{
  std::ostringstream oss;
  oss << _endpoint.address() << "_" << eid;
  std::string sensor_id=oss.str();
	int retry_counter = 0;
  while (retry_counter < 3) {
    retry_counter++;
    try {
      hexanode::Sensor::Ptr sensor = _sensors->get_by_id(sensor_id);
      sensor->post_value(_client, _api_uri, value);
      target << "Sensor " << sensor_id << ": submitted value " << value << std::endl;
			return;
    } catch (const hexanode::NotFoundException& e) {
			// The sensor was not found during the get_by_id call.
			switch (eid) {
				case EP_PV_PRODUCTION:
				case EP_POWER_BALANCE:
				case EP_BATTERY_BALANCE:
					{
						std::cout << "No information regarding sensor " << sensor_id 
							<< " found - creating sensor with boilerplate data." << std::endl;
						int min_value = 0;
						int max_value = 0;
						std::string unit("W");
						std::string name;
						switch(eid) {
							case EP_PV_PRODUCTION:
								min_value = 0;
								max_value = 5000;
								name = "PV Production";
								break;

							case EP_POWER_BALANCE:
								min_value = -10000;
								max_value = 10000;
								name = "Power Balance";
								break;

							case EP_BATTERY_BALANCE:
								min_value = -5000;
								max_value = 5000;
								name = "Battery";
								break;
						}
						hexanode::Sensor::Ptr new_sensor(new hexanode::Sensor(sensor_id, 
									name + " [" + unit + "]",
									min_value, max_value,
									std::string("dashboard")
									));
						_sensors->add_sensor(new_sensor);
					}
					break;

				default:
					boost::asio::ip::address_v6 addr = _endpoint.address().to_v6();
					if (_unidentified_devices.count(addr) == 0) {
						_info.send_request(
								addr,
								hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR),
								hexabus::filtering::isEndpointInfo() && hexabus::filtering::eid() == EP_DEVICE_DESCRIPTOR,
								boost::bind(&PacketPusher::deviceInfoReceived, this, addr, _1),
								boost::bind(&PacketPusher::deviceInfoError, this, addr, _1));
						_unidentified_devices.insert(std::make_pair(_endpoint.address().to_v6(), std::set<uint32_t>()));
					}
					_unidentified_devices[addr].insert(eid);
					break;
			}
    } catch (const hexanode::CommunicationException& e) {
      // An error occured during network communication.
      std::cout << "Cannot submit sensor values (" << e.what() << ")" << std::endl;
      std::cout << "Attempting to define sensor at the frontend cache." << std::endl;
      hexanode::Sensor::Ptr sensor=_sensors->get_by_id(sensor_id);
      sensor->put(_client, _api_uri, "n/a"); 
    } catch (const std::exception& e) {
      target << "Attempting to recover from error: " << e.what() << std::endl;
    }
  }
	target << "FAILED to submit sensor value." << std::endl;
}


void PacketPusher::printValueHeader(uint32_t eid, const char* datatypeStr)
{
  target << " EID " << eid;
}



void PacketPusher::printValuePacket(const hexabus::ValuePacket<uint8_t>& packet, const char* datatypeStr)
{
  printValueHeader(packet.eid(), datatypeStr);
  target << "Value:\t" << (int) packet.value() << std::endl;
  target << std::endl;
}



void PacketPusher::visit(const hexabus::ErrorPacket& error)
{
  target << "Error" << std::endl
    << "Error code:\t";
  switch (error.code()) {
    case HXB_ERR_UNKNOWNEID:
      target << "Unknown EID";
      break;
    case HXB_ERR_WRITEREADONLY:
      target << "Write on readonly endpoint";
      break;
    case HXB_ERR_CRCFAILED:
      target << "CRC failed";
      break;
    case HXB_ERR_DATATYPE:
      target << "Datatype mismatch";
      break;
    case HXB_ERR_INVALID_VALUE:
      target << "Invalid value";
      break;
    default:
      target << "(unknown)";
      break;
  }
  target << std::endl;
  target << std::endl;
}

void PacketPusher::visit(const hexabus::QueryPacket& query) {}
void PacketPusher::visit(const hexabus::EndpointQueryPacket& endpointQuery) {}

void PacketPusher::visit(const hexabus::EndpointInfoPacket& endpointInfo)
{
  if (endpointInfo.eid() == 0) {
    target << "Device Info" << std::endl
      << "Device Name:\t" << endpointInfo.value() << std::endl;
  } else {
    target << "Endpoint Info\n"
      << "Endpoint ID:\t" << endpointInfo.eid() << std::endl
      << "EP Datatype:\t";
    switch(endpointInfo.datatype()) {
      case HXB_DTYPE_BOOL:
        target << "Bool";
        break;
      case HXB_DTYPE_UINT8:
        target << "UInt8";
        break;
      case HXB_DTYPE_UINT32:
        target << "UInt32";
        break;
      case HXB_DTYPE_DATETIME:
        target << "Datetime";
        break;
      case HXB_DTYPE_FLOAT:
        target << "Float";
        break;
      case HXB_DTYPE_TIMESTAMP:
        target << "Timestamp";
        break;
      case HXB_DTYPE_128STRING:
        target << "String";
        break;
      case HXB_DTYPE_16BYTES:
        target << "Binary (16bytes)";
        break;
      case HXB_DTYPE_66BYTES:
        target << "Binary (66bytes)";
        break;
      default:
        target << "(unknown)";
        break;
    }
    target << std::endl;
    target << "EP Name:\t" << endpointInfo.value() << std::endl;
  }
  target << std::endl;
}


