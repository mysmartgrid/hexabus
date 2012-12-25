#include <iostream>
#include <fstream>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/tempsensor.hpp>

// libklio includes. TODO: create support for built-system.
#include <libklio/common.hpp>
#include <sstream>
#include <libklio/store.hpp>
#include <libklio/store-factory.hpp>
#include <libklio/sensor.hpp>
#include <libklio/sensorfactory.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
// commandline parsing.
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;

#pragma GCC diagnostic warning "-Wstrict-aliasing"

// TODO: Remove this, only for debugging.
using namespace boost::posix_time;
#include <unistd.h>


#include "../../shared/hexabus_packet.h"


struct ReadingLogger : private hexabus::PacketVisitor {
	private:
		klio::Store::Ptr store;
		klio::TimeConverter::Ptr tc;
		klio::SensorFactory::Ptr sensor_factory;
		std::string sensor_timezone;

		boost::asio::ip::address_v6 source;
		float packetValue;

		const char* eidToUnit(uint32_t eid)
		{
			switch (eid) {
				case 1:
				case 4:
				case 23:
				case 24:
				case 25:
				case 26:
					return "boolean";
				case 2:
					return "Watt";
				case 3:
					return "deg Celsius";
				case 5:
					return "% r.h.";
				case 6:
					return "hPa";
				default:
					return "unknown";
			}
		}

		void rejectPacket()
		{
			std::cout << "Received some packet." << std::endl;
		}

		void acceptPacket(float reading, uint32_t eid)
		{
			/**
			 * 1. Create unique ID for each info message and sensor,
			 * <ip>+<endpoint>
			 */
			std::ostringstream oss;
			oss << source.to_string() << "-" << eid;
			std::string sensor_id(oss.str());
			std::cout << "Received a reading from " << sensor_id << ", value "
				<< reading << std::endl;

			/**
			 * 2. Ask Klio for a sensor instance. If none is known for this
			 * sensor, create a new one.
			 */

			try {
				bool found=false;
				std::vector<klio::Sensor::uuid_t> uuids = store->getSensorUUIDs();
				std::vector<klio::Sensor::uuid_t>::iterator it;
				for(  it = uuids.begin(); it < uuids.end(); it++) {
					klio::Sensor::Ptr loadedSensor(store->getSensor(*it));
					if (boost::iequals(loadedSensor->name(), sensor_id)) {
						// We have found our sensor. Now add data to it.
						found=true;
						/**
						 * 3. Use the sensor instance to save the value.
						 */
						klio::timestamp_t timestamp=tc->get_timestamp();
						store->add_reading(loadedSensor, timestamp, reading);
						//std::cout << "Added reading to sensor " 
						//  << loadedSensor->name() << std::endl;
						break;
					}
				}
				if (! found) {
					// apparently, this is a new sensor. Create a representation in klio for it.
					klio::Sensor::Ptr new_sensor(sensor_factory->createSensor(
								sensor_id, eidToUnit(eid), sensor_timezone)); 
					store->addSensor(new_sensor);
					std::cout << "Created new sensor: " << new_sensor->str() << std::endl;
					/**
					 * 3. Use the sensor instance to save the value.
					 */
					klio::timestamp_t timestamp=tc->get_timestamp();
					store->add_reading(new_sensor, timestamp, reading);
					std::cout << "Added reading to sensor " 
						<< new_sensor->name() << std::endl;
				} 
			} catch (klio::StoreException const& ex) {
				std::cout << "Failed to record reading: " << ex.what() << std::endl;
			} catch (std::exception const& ex) {
				std::cout << "Failed to record reading: " << ex.what() << std::endl;
			}
		}

		virtual void visit(const hexabus::ErrorPacket& error) { rejectPacket(); }
		virtual void visit(const hexabus::QueryPacket& query) { rejectPacket(); }
		virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) { rejectPacket(); }
		virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo) { rejectPacket(); }

		virtual void visit(const hexabus::InfoPacket<bool>& info)
		{
			acceptPacket(info.value(), info.eid());
		}
		
		virtual void visit(const hexabus::InfoPacket<uint8_t>& info)
		{
			acceptPacket(info.value(), info.eid());
		}
		
		virtual void visit(const hexabus::InfoPacket<uint32_t>& info)
		{
			acceptPacket(info.value(), info.eid());
		}

		virtual void visit(const hexabus::InfoPacket<float>& info)
		{
			acceptPacket(info.value(), info.eid());
		}

		virtual void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info)
		{
			// TODO: handle this properly
		}

		virtual void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info)
		{
			acceptPacket(info.value().total_seconds(), info.eid());
		}

		virtual void visit(const hexabus::InfoPacket<std::string>& info)
		{
			// TODO: handle this properly
		}

		virtual void visit(const hexabus::InfoPacket<std::vector<char> >& info)
		{
			// TODO: handle this properly
		}


		virtual void visit(const hexabus::WritePacket<bool>& write) { rejectPacket(); }
		virtual void visit(const hexabus::WritePacket<uint8_t>& write) { rejectPacket(); }
		virtual void visit(const hexabus::WritePacket<uint32_t>& write) { rejectPacket(); }
		virtual void visit(const hexabus::WritePacket<float>& write) { rejectPacket(); }
		virtual void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) { rejectPacket(); }
		virtual void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) { rejectPacket(); }
		virtual void visit(const hexabus::WritePacket<std::string>& write) { rejectPacket(); }
		virtual void visit(const hexabus::WritePacket<std::vector<char> >& write) { rejectPacket(); }

	public:
		void operator()(const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet)
		{
			source = addr;
			visitPacket(packet);
		}

		ReadingLogger(const klio::Store::Ptr& store,
				const klio::TimeConverter::Ptr& tc,
				const klio::SensorFactory::Ptr& sensor_factory,
				const std::string& sensor_timezone)
			: store(store), tc(tc), sensor_factory(sensor_factory), sensor_timezone(sensor_timezone)
		{
		}
};


int main(int argc, char** argv)
{
  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " [-s] storefile";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version,v", "print libklio version and exit")
    ("storefile,s", po::value<std::string>(), "the data store to use")
    ("timezone,t", po::value<std::string>(), "the timezone to use for new sensors")
    ;
  po::positional_options_description p;
  p.add("storefile", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
      options(desc).positional(p).run(), vm);
  po::notify(vm);

  // Begin processing of commandline parameters.
  std::string storefile;
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }
  if (vm.count("version")) {
    klio::VersionInfo::Ptr vi(new klio::VersionInfo());
    hexabus::VersionInfo versionInfo;
    std::cout << "libhexabus version " << versionInfo.getVersion() << std::endl;
    std::cout << "klio library version " << vi->getVersion() << std::endl;
    return 0;
  }
  if (! vm.count("storefile")) {
    std::cerr << "You must specify a store to work on." << std::endl;
    return 1;
  } else {
    storefile=vm["storefile"].as<std::string>();
  }
  bfs::path db(storefile);
  if (! bfs::exists(db)) {
    std::cerr << "Database " << db << " does not exist, cannot continue." << std::endl;
    std::cerr << "Hint: you can create a database using klio-store create <dbfile>" << std::endl;
    return 2;
  }

  std::string sensor_timezone("Europe/Berlin"); 
  if (! vm.count("timezone")) {
    std::cerr << "Using default timezone " << sensor_timezone 
      << ", change with -t <NEW_TIMEZONE>" << std::endl;
  } else {
    sensor_timezone=vm["timezone"].as<std::string>();
  }



  std::map<std::string, hexabus::Sensor::Ptr> sensors;
	boost::asio::io_service io;
  hexabus::Socket network(io, hexabus::Socket::Unreliable);
  // TODO: Compile flag etc.
  klio::StoreFactory::Ptr store_factory(new klio::StoreFactory()); 
  klio::Store::Ptr store(store_factory->openStore(klio::SQLITE3, db));
  std::cout << "opened store: " << store->str() << std::endl;
  klio::SensorFactory::Ptr sensor_factory(new klio::SensorFactory());
  klio::TimeConverter::Ptr tc(new klio::TimeConverter());

	network.onPacketReceived(ReadingLogger(store, tc, sensor_factory, sensor_timezone));
	network.run();
}
