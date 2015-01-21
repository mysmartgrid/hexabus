#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string.h>
#include <libhexabus/common.hpp>
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
#include <libklio/sqlite3/sqlite3-store.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>

// commandline parsing.
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;

#include <boost/format.hpp>
//namespace bf = boost::format;

#pragma GCC diagnostic warning "-Wstrict-aliasing"

#include <unistd.h>

#include <libhexabus/logger/logger.hpp>

#include "resolv.hpp"
using boost::format;
using boost::io::group;

#define KLIO_AUTOCOMMIT 0

class Logger : public hexabus::Logger {
private:
  bfs::path store_file;
  klio::SQLite3Store::Ptr store;
  unsigned int _packet_counter;
  unsigned int _packet_counter_max;

  klio::Sensor::Ptr lookup_sensor(const std::string& id)
		{
			std::vector<klio::Sensor::Ptr> sensors = store->get_sensors_by_external_id(id);

			if (!sensors.size())
				return klio::Sensor::Ptr();
      return sensors[0];
		}

  void new_sensor_found(klio::Sensor::Ptr sensor, const boost::asio::ip::address_v6&)
		{
			store->add_sensor(sensor);
			std::cout << "Created new sensor: " << sensor->str() << std::endl;
#if KLIO_AUTOCOMMIT
      store->commit_transaction();
      store->start_transaction();
#endif
		}

  void record_reading(klio::Sensor::Ptr sensor, klio::timestamp_t ts, double value)
		{
      double msecs1;
      double msecs2;
      double mdiff;

      struct timeval tv1;
      struct timeval tv2;
#if KLIO_AUTOCOMMIT
      if( _packet_counter > _packet_counter_max ) {
        _packet_counter = 0;
        std::cout<<"need to commit fitst"<< std::endl;
        store->commit_transaction();
        store->start_transaction();
      }
#endif
			try {

        gettimeofday(&tv1, NULL) ;
        msecs1=(double)tv1.tv_sec + ((double)tv1.tv_usec/1000000);
				std::cout << "Call Record-Reading at " <<  tv1.tv_sec << "." << tv1.tv_usec << std::endl;

				store->add_reading(sensor, ts, value);
        _packet_counter++;
        gettimeofday(&tv2, NULL) ;
        msecs2=(double)tv2.tv_sec + ((double)tv2.tv_usec/1000000);
        mdiff = msecs2 - msecs1;

				std::cout << "Added reading " << value << " to sensor " << sensor->name() << " ("<<sensor->external_id()<< ")" << "t="<< ts << " diff=" <<mdiff << std::endl;
			} catch (klio::StoreException const& ex) {
        gettimeofday(&tv2, NULL) ;
        msecs2=(double)tv2.tv_sec + ((double)tv2.tv_usec/1000000);
        mdiff = msecs2 - msecs1;

				std::cout << "Failed to record reading to sensor " << sensor->name()<< " ("<<sensor->external_id()<< ")"<< "t="<< ts << " diff=" << mdiff<< " : " << ex.what() << std::endl;
			} catch (std::exception const& ex) {
        gettimeofday(&tv2, NULL) ;
        msecs2=(double)tv2.tv_sec + ((double)tv2.tv_usec/1000000);
        mdiff = msecs2 - msecs1;

				std::cout << "Failed to record reading to sensor " << sensor->name()<< " ("<<sensor->external_id()<< ")"<< "t="<< ts << " diff=" << mdiff<< " : " << ex.what() << std::endl;
			}
		}

public:
  Logger(const bfs::path& store_file,
         klio::SQLite3Store::Ptr& store,
         klio::TimeConverter& tc,
         klio::SensorFactory& sensor_factory,
         const std::string& sensor_timezone,
         hexabus::DeviceInterrogator& interrogator,
         hexabus::EndpointRegistry& reg)
			: hexabus::Logger(tc, sensor_factory, sensor_timezone, interrogator, reg), store_file(store_file), store(store)
      , _packet_counter(0)
      , _packet_counter_max(10)
		{
		}

  void rotate_stores()
		{
			std::cout << "Rotating store " << store_file << "..." << std::endl;
			std::cout << "Rotating store call flush " << store_file << "..." << std::endl;
      fflush(stdout);
			try {
        store->flush(true);
			} catch (klio::StoreException const& ex) {
				std::cout << "Failed to flush the buffers : " << ex.what() << std::endl;
      }
      std::cout << "Rotating store flushed " << store_file << "..." << std::endl;
      fflush(stdout);

			std::cout << "Reopening store" << std::endl;

      const boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();

      std::string s;
      s= str( format("%04d%02d%02d-%02d%02d")        % now.date().year_month_day().year
              % now.date().year_month_day().month.as_number()
              % now.date().year_month_day().day.as_number()
              % now.time_of_day().hours()
              % now.time_of_day().minutes());

      std::string name(store_file.string());
      name+=".";
      name+=s;

      bfs::path dbname(name);
      std::cout << "===> renaming to: "<< name<<std::endl;
      fflush(stdout);
			try {
        store->rotate(dbname);
			} catch (klio::StoreException const& ex) {
				std::cout << "Failed to rotate the klio-databse : " << ex.what() << std::endl;
      }


#if KLIO_AUTOCOMMIT
      store->start_transaction();
#endif

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
#if KLIO_AUTOCOMMIT
		store = store_factory.open_sqlite3_store(db, false, true, 30);
    store->start_transaction();
#else
		store = store_factory.open_sqlite3_store(db, true, true, 30, klio::SQLite3Store::OS_SYNC_OFF);
#endif

		std::string sensor_timezone("Europe/Berlin");
		if (! vm.count("timezone")) {
			std::cerr << "Using default timezone " << sensor_timezone
			<< ", change with -t <NEW_TIMEZONE>" << std::endl;
		} else {
			sensor_timezone=vm["timezone"].as<std::string>();
		}

		hexabus::Listener listener(io);

		hexabus::Socket network(io);
		std::cout << "opened store: " << store->str() << std::endl;
		klio::SensorFactory sensor_factory;
		klio::TimeConverter tc;
		hexabus::DeviceInterrogator di(network);
		hexabus::EndpointRegistry reg;

		Logger logger(storefile, store, tc, sensor_factory, sensor_timezone, di, reg);

		network.bind(addr);
		listener.listen(interface);
		listener.onPacketReceived(boost::ref(logger));

		boost::asio::signal_set rotate_handler(io, SIGHUP);
		boost::asio::signal_set terminate_handler(io, SIGTERM);

    bool term_received = false;
		while (!term_received) {
			bool hup_received = false;

			using namespace boost::lambda;
			rotate_handler.async_wait(
        (var(hup_received) = true,
         boost::lambda::bind(&boost::asio::io_service::stop, &io)));
			terminate_handler.async_wait(
        (var(term_received) = true,
         boost::lambda::bind(&boost::asio::io_service::stop, &io)));

			io.reset();
			io.run();

			if (hup_received) {
				logger.rotate_stores();
			} else {
				break;
			}
		}
    std::cout << "Terminating hexalog."<< std::endl;
    store->flush(true);
    store->close();
    fflush(stdout);

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
