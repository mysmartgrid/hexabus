#include <iostream>
#include <fstream>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>

#include <libklio/common.hpp>
#include <sstream>
#include <libklio/store.hpp>
#include <libklio/store-factory.hpp>
#include <libklio/sensor.hpp>
#include <libklio/sensor-factory.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
// commandline parsing.
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;

#pragma GCC diagnostic warning "-Wstrict-aliasing"

#include <unistd.h>

#include "resolv.hpp"


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
				std::vector<klio::Sensor::uuid_t> uuids = store->get_sensor_uuids();
				std::vector<klio::Sensor::uuid_t>::iterator it;
				for(  it = uuids.begin(); it < uuids.end(); it++) {
					klio::Sensor::Ptr loadedSensor(store->get_sensor(*it));
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
					store->add_sensor(new_sensor);
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

		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info)
		{
			// TODO: handle this properly
		}

		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info)
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
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) { rejectPacket(); }
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) { rejectPacket(); }

	public:
		void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& from)
		{
			source = from.address().to_v6();
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


enum ErrorCode {
	ERR_NONE = 0,

	ERR_UNKNOWN_PARAMETER = 1,
	ERR_PARAMETER_MISSING = 2,
	ERR_PARAMETER_FORMAT = 3,
	ERR_PARAMETER_VALUE_INVALID = 4,
	ERR_NETWORK = 5,

	ERR_KLIO = 6,

	ERR_OTHER = 127
};


int main(int argc, char** argv)
{
	std::ostringstream oss;
 	oss << "Usage: " << argv[0] << " [-s] <interface> <storefile>";
 	po::options_description desc(oss.str());
 	desc.add_options()
   	("help,h", "produce help message")
	("version,v", "print libklio version and exit")
	("storefile,s", po::value<std::string>(), "the data store to use")
	("timezone,t", po::value<std::string>(), "the timezone to use for new sensors")
		("interface,I", po::value<std::string>(), "interface to listen on")
		("bind,b", po::value<std::string>(), "address to bind to")
	;
  	po::positional_options_description p;
  	p.add("interface", 1);
  	p.add("storefile", 1);

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).
                    options(desc).positional(p).run(), vm);
		po::notify(vm);
	} catch (const std::exception& e) {
		std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
		return ERR_UNKNOWN_PARAMETER;
	}

	// Begin processing of commandline parameters.
	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return ERR_NONE;
	}

  	// TODO: Compile flag etc.
  	if (vm.count("version")) {
		klio::VersionInfo::Ptr vi(new klio::VersionInfo());
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		std::cout << "klio library version " << vi->getVersion() << std::endl;
		return ERR_NONE;
  	}

	if (!vm.count("interface")) {
		std::cerr << "You must specify an interface to listen on." << std::endl;
		return ERR_PARAMETER_MISSING;
	}
	if (!vm.count("storefile")) {
		std::cerr << "You must specify a store to work on." << std::endl;
		return ERR_PARAMETER_MISSING;
	}

	std::string interface(vm["interface"].as<std::string>());
	boost::asio::ip::address_v6 addr(boost::asio::ip::address_v6::any());
	boost::asio::io_service io;

	if (vm.count("bind")) {
		boost::system::error_code err;

		addr = hexabus::resolve(io, vm["bind"].as<std::string>(), err);
		if (err) {
			std::cerr << vm["bind"].as<std::string>() << " is not a valid IP address: " << err.message() << std::endl;
			return ERR_PARAMETER_FORMAT;
		}
	}

	try {
		klio::StoreFactory::Ptr store_factory(new klio::StoreFactory()); 
		klio::Store::Ptr store;

                std::string storefile(vm["storefile"].as<std::string>());
                bfs::path db(storefile);
                if (! bfs::exists(db)) {
                        std::cerr << "Database " << db << " does not exist, cannot continue." << std::endl;
                        std::cerr << "Hint: you can create a database using klio-store create <dbfile>" << std::endl;
                        return ERR_PARAMETER_VALUE_INVALID;
                }
                store = store_factory->create_sqlite3_store(db);

		std::string sensor_timezone("Europe/Berlin"); 
		if (! vm.count("timezone")) {
			std::cerr << "Using default timezone " << sensor_timezone 
			<< ", change with -t <NEW_TIMEZONE>" << std::endl;
		} else {
			sensor_timezone=vm["timezone"].as<std::string>();
		}

		hexabus::Socket network(io, interface);
		std::cout << "opened store: " << store->str() << std::endl;
		klio::SensorFactory::Ptr sensor_factory(new klio::SensorFactory());
		klio::TimeConverter::Ptr tc(new klio::TimeConverter());

		network.listen(addr);
		network.onPacketReceived(ReadingLogger(store, tc, sensor_factory, sensor_timezone));

		io.run();
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Network error: " << e.code().message() << std::endl;
		return ERR_NETWORK;
	} catch (const klio::GenericException& e) {
		std::cerr << "Klio error: " << e.reason() << std::endl;
		return ERR_KLIO;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return ERR_OTHER;
	}
}
