#include <iostream>
#include <fstream>
#include <map>
#include <set>
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

#include <libhexabus/logger/logger.hpp>

#include "resolv.hpp"

class Logger : public hexabus::Logger, virtual hexabus::TypedPacketVisitor<hexabus::ReportPacket>::Empty {
	private:
		bfs::path store_file;
		klio::Store::Ptr store;

		klio::Sensor::Ptr lookup_sensor(const std::string& id)
		{
			try {
				return store->get_sensor_by_external_id(id);
			} catch (...) {
				return klio::Sensor::Ptr();
			}
		}

		void new_sensor_found(klio::Sensor::Ptr sensor, const boost::asio::ip::address_v6&)
		{
			store->add_sensor(sensor);
			std::cout << "Created new sensor: " << sensor->str() << std::endl;
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

	public:
		Logger(const bfs::path& store_file,
			klio::Store::Ptr& store,
			klio::TimeConverter& tc,
			klio::SensorFactory& sensor_factory,
			const std::string& sensor_timezone,
			hexabus::DeviceInterrogator& interrogator,
			hexabus::EndpointRegistry& reg)
			: hexabus::Logger(tc, sensor_factory, sensor_timezone, interrogator, reg), store_file(store_file), store(store)
		{
		}

		void rotate_stores()
		{
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

			std::cout << "Reopening store" << std::endl;
			store->close();
			store = klio::StoreFactory().create_sqlite3_store(store_file);
			store->open();
			store->initialize();

			{
				std::cout << "Adding sensors..." << std::endl;
				std::set<klio::Sensor::uuid_t> new_sensors;
				{
					std::vector<klio::Sensor::uuid_t> uuids = store->get_sensor_uuids();
					new_sensors.insert(uuids.begin(), uuids.end());
				}

				std::map<std::string, klio::Sensor::Ptr>::const_iterator it, end;
				for (it = sensor_cache.begin(), end = sensor_cache.end(); it != end; ++it) {
					if (!new_sensors.count(it->second->uuid())) {
						store->add_sensor(it->second);
						std::cout << it->second->name() << std::endl;
					}
				}
			}

			std::cout << "Rotation done" << std::endl;
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
		hexabus::DeviceInterrogator di(network);
		hexabus::EndpointRegistry reg;

		Logger logger(storefile, store, tc, sensor_factory, sensor_timezone, di, reg);

		if (vm.count("bind")) {
			network.bind(boost::asio::ip::udp::endpoint(addr, HXB_PORT));
		} else {
			network.listen();
		}
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
