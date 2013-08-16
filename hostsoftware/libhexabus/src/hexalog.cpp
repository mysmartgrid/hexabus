#include <iostream>
#include <fstream>
#include <map>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/device_interrogator.hpp>
#include <libhexabus/endpoint_registry.hpp>

#include <libklio/common.hpp>
#include <sstream>
#include <libklio/store.hpp>
#include <libklio/store-factory.hpp>
#include <libklio/sensor.hpp>
#include <libklio/sensor-factory.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
// commandline parsing.
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;

#pragma GCC diagnostic warning "-Wstrict-aliasing"

#include <unistd.h>

#include "resolv.hpp"
#include "../../../shared/endpoints.h"

class Logger : private hexabus::PacketVisitor {
	private:
		bfs::path store_file;
		klio::Store::Ptr store;
		klio::TimeConverter& tc;
		klio::SensorFactory& sensor_factory;
		std::string sensor_timezone;
		hexabus::DeviceInterrogator interrogator;

		hexabus::EndpointRegistry registry;

		typedef std::pair<uint32_t, klio::readings_t> new_sensor_t;
		std::map<std::string, new_sensor_t> new_sensor_backlog;
		std::map<std::string, klio::Sensor::Ptr> sensor_cache;

		boost::asio::ip::address_v6 source;

		static std::string to_string(uint32_t u)
		{
			std::stringstream o;
			o << u;
			return o.str();
		}

		const char* eid_to_unit(uint32_t eid)
		{
			hexabus::EndpointRegistry::const_iterator it;

			it = registry.find(eid);
			if (it == registry.end() || !it->second.unit()) {
				return "unknown";
			} else {
				return it->second.unit()->c_str();
			}
		}

		void record_reading(klio::Sensor::Ptr sensor, klio::timestamp_t ts, double value)
		{
			try {
				store->add_reading(sensor, ts, value);
				std::cout << "Added reading " << value << " to sensor " << sensor->name() << std::endl;
			} catch (klio::StoreException const& ex) {
				std::cout << "Failed to record reading: " << ex.what() << std::endl;
			} catch (std::exception const& ex) {
				std::cout << "Failed to record reading: " << ex.what() << std::endl;
			}
		}

		void on_sensor_name_received(const std::string& sensor_name, const hexabus::Packet& ep_info)
		{
			klio::Sensor::Ptr sensor = sensor_factory.createSensor(
					static_cast<const hexabus::EndpointInfoPacket&>(ep_info).value(),
					sensor_name,
					eid_to_unit(new_sensor_backlog[sensor_name].first),
					sensor_timezone);

			store->add_sensor(sensor);
			sensor_cache.insert(std::make_pair(sensor_name, sensor));
			std::cout << "Created new sensor: " << sensor->str() << std::endl;

			new_sensor_t& backlog = new_sensor_backlog[sensor_name];
			klio::readings_it_t it, end;
			for (it = backlog.second.begin(), end = backlog.second.end(); it != end; ++it) {
				record_reading(sensor, it->first, it->second);
			}

			new_sensor_backlog.erase(sensor_name);
		}

		void on_sensor_error(const std::string& sensor_name, const hexabus::GenericException& err)
		{
			std::cout
				<< "Error getting device name: " << err.what() << ", "
				<< "dropping " << new_sensor_backlog[sensor_name].second.size() 
				<< " readings from " << sensor_name << std::endl;

			new_sensor_backlog.erase(sensor_name);
		}

		void accept_packet(double value, uint32_t eid)
		{
			/**
			 * 1. Create unique ID for each info message and sensor,
			 * <ip>-<endpoint>
			 */
			std::ostringstream oss;
			oss << source.to_string() << "-" << eid;
			std::string sensor_name(oss.str());
			std::cout << "Received a reading from " << sensor_name << ", value "
				<< value << std::endl;

			/**
			 * 2. Ask cache/Klio for a sensor instance. If none is known for this
			 * sensor, create a new one.
			 */

			klio::Sensor::Ptr sensor;
			klio::timestamp_t now = tc.get_timestamp();

			if (sensor_cache.count(sensor_name)) {
				sensor = sensor_cache[sensor_name];
			} else {
				std::vector<klio::Sensor::Ptr> sensors = store->get_sensors_by_name(sensor_name);
				if (sensors.size()) {
					sensor = sensors[0];
				}
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

		virtual void visit(const hexabus::InfoPacket<bool>& info)
		{
			accept_packet(info.value(), info.eid());
		}
		
		virtual void visit(const hexabus::InfoPacket<uint8_t>& info)
		{
			accept_packet(info.value(), info.eid());
		}
		
		virtual void visit(const hexabus::InfoPacket<uint32_t>& info)
		{
			accept_packet(info.value(), info.eid());
		}

		virtual void visit(const hexabus::InfoPacket<float>& info)
		{
			accept_packet(info.value(), info.eid());
		}

		virtual void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info)
		{
			accept_packet(info.value().total_seconds(), info.eid());
		}

		// FIXME: handle these properly, we should not drop valid info packets
		virtual void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info) {} 
		virtual void visit(const hexabus::InfoPacket<std::string>& info) {} 
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {} 
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {}

		virtual void visit(const hexabus::ErrorPacket& error) {}
		virtual void visit(const hexabus::QueryPacket& query) {}
		virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) {}
		virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo) {}

		virtual void visit(const hexabus::WritePacket<bool>& write) {}
		virtual void visit(const hexabus::WritePacket<uint8_t>& write) {}
		virtual void visit(const hexabus::WritePacket<uint32_t>& write) {}
		virtual void visit(const hexabus::WritePacket<float>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) {}
		virtual void visit(const hexabus::WritePacket<std::string>& write) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}

	public:
		Logger(const std::string& store_file,
			klio::Store::Ptr& store,
			klio::TimeConverter& tc,
			klio::SensorFactory& sensor_factory,
			const std::string& sensor_timezone,
			hexabus::Socket& socket)
			: store_file(store_file), store(store), tc(tc), sensor_factory(sensor_factory), sensor_timezone(sensor_timezone), interrogator(socket)
		{
		}

		void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& from)
		{
			source = from.address().to_v6();
			visitPacket(packet);
		}

		void rotate_stores()
		{
			try {
				std::cout << "Rotating store " << store_file << "..." << std::endl;

				{
					std::cout << "Loading sensor schema" << std::endl;
					std::vector<klio::Sensor::uuid_t> uuids = store->get_sensor_uuids();
					std::vector<klio::Sensor::uuid_t>::const_iterator it, end;
					for (it = uuids.begin(), end = uuids.end(); it != end; ++it) {
						klio::Sensor::Ptr sensor = store->get_sensor(*it);

						sensor_cache[sensor->name()] = sensor;
					}
				}

				std::cout << "Rotating files" << std::endl;
				store->close();

				bfs::path dir = store_file.parent_path();
				uint32_t rotate_top = 1;
				while (exists(dir / (store_file.filename().native() + "." + to_string(rotate_top)))) {
					rotate_top++;
				}
				while (rotate_top > 1) {
					bfs::path old_name = dir / (store_file.filename().native() + "." + to_string(rotate_top - 1));
					bfs::path new_name = dir / (store_file.filename().native() + "." + to_string(rotate_top));
					std::cout << "Move " << old_name << " -> " << new_name << std::endl;
					rename(old_name, new_name);
					rotate_top--;
				}
				std::cout << "Move " << store_file << " -> " << (dir / (store_file.filename().native() + ".1")) << std::endl;
				rename(store_file, dir / (store_file.filename().native() + ".1"));

				std::cout << "Create new store " << store_file << std::endl;
				store = klio::StoreFactory().create_sqlite3_store(store_file);
				store->open();
				store->initialize();

				std::cout << "Adding sensors..." << std::endl;
				std::map<std::string, klio::Sensor::Ptr>::const_iterator it, end;
				for (it = sensor_cache.begin(), end = sensor_cache.end(); it != end; ++it) {
					store->add_sensor(it->second);
					std::cout << it->second->name() << std::endl;
				}

				std::cout << "Rotation done" << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "Could not rotate: " << e.what() << ", resuming as if nothing happened" << std::endl;
			}
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
	oss << "Usage: " << argv[0] << " <interface> [-s] <storefile>";
	po::options_description desc(oss.str());
	desc.add_options()
		("help,h", "produce help message")
		("version,v", "print version and exit")
		("storefile,s", po::value<std::string>(), "the data store to use")
		("timezone,t", po::value<std::string>(), "the timezone to use for new sensors")
		("interface,I", po::value<std::string>(), "interface to listen on")
		("bind,b", po::value<std::string>(), "address to bind to");

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
		klio::StoreFactory store_factory; 
		klio::Store::Ptr store;

		std::string storefile(vm["storefile"].as<std::string>());
		bfs::path db(storefile);
		if (! bfs::exists(db)) {
			std::cerr << "Database " << db << " does not exist, cannot continue." << std::endl;
			std::cerr << "Hint: you can create a database using klio-store create <dbfile>" << std::endl;
			return ERR_PARAMETER_VALUE_INVALID;
		}
		store = store_factory.open_sqlite3_store(db);

		std::string sensor_timezone("Europe/Berlin"); 
		if (! vm.count("timezone")) {
			std::cerr << "Using default timezone " << sensor_timezone 
			<< ", change with -t <NEW_TIMEZONE>" << std::endl;
		} else {
			sensor_timezone=vm["timezone"].as<std::string>();
		}

		hexabus::Socket network(io, interface);
		std::cout << "opened store: " << store->str() << std::endl;
		klio::SensorFactory sensor_factory;
		klio::TimeConverter tc;

		Logger logger(storefile, store, tc, sensor_factory, sensor_timezone, network);

		network.listen(addr);
		network.onPacketReceived(boost::ref(logger));

		boost::asio::signal_set rotate_handler(io, SIGHUP);

		while (true) {
			bool hup_received = false;

			using namespace boost::lambda;
			rotate_handler.async_wait(
					(var(hup_received) = true,
					boost::lambda::bind(&boost::asio::io_service::stop, &io)));

			io.reset();
			io.run();

			std::cout << "stopped " << hup_received << std::endl;
			if (hup_received) {
				logger.rotate_stores();
			} else {
				break;
			}
		}
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
