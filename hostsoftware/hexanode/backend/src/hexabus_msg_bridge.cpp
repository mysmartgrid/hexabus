#include <iostream>
#include <unistd.h>

#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/optional.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <libklio/store.hpp>
#include <libklio/store-factory.hpp>
#include <libklio/sensor.hpp>
#include <libklio/sensor-factory.hpp>

#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/endpoint_registry.hpp>

#include <libhexabus/logger/logger.hpp>

#include "../../../shared/endpoints.h"

namespace po = boost::program_options;

struct ReadingLogger : public hexabus::Logger {
	private:
		struct SensorInfo {
			boost::posix_time::ptime last_value_received_at;
			boost::posix_time::ptime last_name_checked_at;
			boost::asio::ip::address_v6 address;
		};

		klio::MSGStore::Ptr store;
		std::map<klio::Sensor::Ptr, SensorInfo> sensor_infos;
		boost::asio::deadline_timer info_timer;
		boost::asio::deadline_timer flush_timer;
		
		static const char* UNKNOWN_UNIT;

		std::string get_sensor_id(const boost::asio::ip::address_v6& source, uint32_t eid)
		{
			std::stringstream id;

			boost::asio::ip::address_v6::bytes_type bytes = source.to_bytes();
			id << std::hex << std::setfill('0');
			for (size_t i = 8; i < 16; ++i) {
				id << std::setw(2) << (0xFF & bytes[i]);
				if (i < 15) {
					id << ":";
				}
			}
			id << std::dec << "-" << eid;
			return id.str();
		}

		const char* eid_to_unit(uint32_t eid)
		{
			std::string unit(hexabus::Logger::eid_to_unit(eid));

			if (unit == "kWh")
				return "kWh";
			else if (unit == "degC")
				return "degC";
			else if (unit == "%r.h.")
				return "rh";
			else if (unit == "hPa")
				return "hPa";
			else if (unit == "_hsbs")
				return "_hsbs";
			else
				return UNKNOWN_UNIT;
		}

		klio::Sensor::Ptr lookup_sensor(const std::string& id)
		{
			try {
				klio::Sensor::Ptr ptr = store->get_sensor_by_external_id(id);

				if (ptr) {
					std::string addr_str = id.substr(0, id.find_first_of('-'));

					SensorInfo info = {
						boost::posix_time::second_clock::local_time(),
						boost::posix_time::second_clock::local_time(),
						boost::asio::ip::address_v6::from_string(addr_str)
					};
					sensor_infos.insert(std::make_pair(ptr, info));
				}
				return ptr;
			} catch (const klio::StoreException& e) {
				if (e.what() == "Sensor " + id + " could not be found.") {
					return klio::Sensor::Ptr();
				} else {
					throw;
				}
			}
		}

		void new_sensor_found(klio::Sensor::Ptr sensor, const boost::asio::ip::address_v6& address)
		{
			if (sensor->unit() == UNKNOWN_UNIT)
				return;

			store->add_sensor(sensor);
			SensorInfo info = {
				boost::posix_time::second_clock::local_time(),
				boost::posix_time::second_clock::local_time(),
				address
			};
			sensor_infos.insert(std::make_pair(sensor, info));
			std::cout << "Created new sensor: " << sensor->str() << std::endl;
		}

		void record_reading(klio::Sensor::Ptr sensor, klio::timestamp_t ts, double value)
		{
			if (sensor->unit() == UNKNOWN_UNIT)
				return;

			sensor_infos[sensor].last_value_received_at = boost::posix_time::second_clock::local_time();

			try {
				store->add_reading(sensor, ts, value);
				std::cout << "Added reading " << value << " to sensor " << sensor->name() << std::endl;
			} catch (std::exception const& ex) {
				std::cout << "Failed to record reading: " << ex.what() << std::endl;
				throw;
			}
		}

		void schedule_info_update()
		{
			info_timer.expires_from_now(boost::posix_time::minutes(1));
			info_timer.async_wait(boost::bind(&ReadingLogger::timer_expired, this, _1));
		}

		void timer_expired(const boost::system::error_code& err)
		{
			using namespace boost::posix_time;

			schedule_info_update();
			if (!err) {
				typedef std::map<klio::Sensor::Ptr, SensorInfo>::iterator iter_t;

				iter_t item = sensor_infos.end();
				for (iter_t it = sensor_infos.begin(), end = sensor_infos.end(); it != end; it++) {
					if (item == end
							|| it->second.last_name_checked_at < item->second.last_name_checked_at) {
						item = it;
					}
				}
				if (item == sensor_infos.end()) {
					return;
				}

				SensorInfo& oldest = item->second;

				if (second_clock::local_time() - oldest.last_name_checked_at >= hours(1)) {
					oldest.last_name_checked_at = second_clock::local_time();
					interrogator.send_request(
							oldest.address,
							hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR),
							hexabus::filtering::IsEndpointInfo(),
							boost::bind(&ReadingLogger::on_sensor_name_received, this, item->first, _1),
							boost::bind(&ReadingLogger::on_sensor_error, this, item->first, _1));
				}
			}
		}

		void on_sensor_name_received(const klio::Sensor::Ptr& sensor, const hexabus::Packet& ep_info)
		{
			sensor->name(static_cast<const hexabus::EndpointInfoPacket&>(ep_info).value());
			store->update_sensor(sensor);
		}

		void on_sensor_error(const klio::Sensor::Ptr& sensor, const hexabus::GenericException& err)
		{
		}

		void schedule_flush()
		{
			flush_timer.expires_from_now(boost::posix_time::minutes(5));
			flush_timer.async_wait(boost::bind(&ReadingLogger::force_flush, this, _1));
		}

		void force_flush(const boost::system::error_code& err)
		{
			schedule_flush();

			if (!err) {
				store->flush();
			}
		}

	public:
		ReadingLogger(boost::asio::io_service& io,
			klio::TimeConverter& tc,
			klio::SensorFactory& sensor_factory,
			const std::string& sensor_timezone,
			hexabus::DeviceInterrogator& interrogator,
			hexabus::EndpointRegistry& registry,
			klio::MSGStore::Ptr store)
			: Logger(tc, sensor_factory, sensor_timezone, interrogator, registry), store(store),
			  info_timer(io), flush_timer(io)
		{
			schedule_info_update();
			schedule_flush();
		}
};
const char* ReadingLogger::UNKNOWN_UNIT = "unknown";

class BridgeConfiguration {
	private:
		std::string _url;
		std::string _device_id;
		std::string _device_key;

	public:
		BridgeConfiguration(const std::string& url, const std::string& device_id, const std::string& device_key)
			: _url(url), _device_id(device_id), _device_key(device_key)
		{
		}

		const std::string& url() const { return _url; }
		const std::string& device_id() const { return _device_id; }
		const std::string& device_key() const { return _device_key; }
};

BridgeConfiguration load_config(const std::string& path)
{
	using namespace boost::property_tree;

	try {
		ptree tree;
		read_ini(path, tree);

		return BridgeConfiguration(
				tree.get<std::string>("bridge.url"),
				tree.get<std::string>("bridge.device_id"),
				tree.get<std::string>("bridge.device_key"));
	} catch (const std::exception& e) {
		std::cerr << "Failed to read configuration file: " << e.what() << std::endl;
		throw;
	}
}

void save_config(const std::string& path, const BridgeConfiguration& config)
{
	using namespace boost::property_tree;

	ptree tree;
	tree.put("bridge.url", config.url());
	tree.put("bridge.device_id", config.device_id());
	tree.put("bridge.device_key", config.device_key());

	try {
		write_ini(path, tree);
	} catch (const std::exception& e) {
		std::cerr << "Failed to write configuration file: " << e.what() << std::endl;
		throw;
	}
}



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

static const char* STORE_TYPE = "raspberrypi";
static const char* STORE_DESCRIPTION = "Hexabus sensor log";

static ErrorCode create_new_store(const std::string&  config, const std::string& url)
{
	boost::uuids::random_generator new_uuid;
	klio::StoreFactory store_factory;
	klio::MSGStore::Ptr store;

	store = store_factory.create_msg_store(
			url,
			to_string(new_uuid()),
			to_string(new_uuid()),
			STORE_DESCRIPTION,
			STORE_TYPE);
	store->initialize();

	BridgeConfiguration store_config(
			store->url(),
			store->id(),
			store->key());
	save_config(config, store_config);

	std::cout << "Created store: " << store->str() << std::endl <<
		std::endl <<
		"Please visit http://www.mysmartgrid.de/device/mylist and " <<
		"enter the activation code " << store->activation_code() <<
		" to link this store to your account." << std::endl <<
		std::endl;

	return ERR_NONE;
}

int main(int argc, char** argv)
{
	std::ostringstream oss;
	oss << "Usage: " << argv[0] << " [ -c <configuration file> ] { -L <interface> | -C [url] | -A | -H }";
	po::options_description desc(oss.str());

	desc.add_options()
		("help,h", "produce help message")
		("version,v", "print version info and exit")
		("config,c", po::value<std::string>()->default_value("/etc/hexabus_msg_bridge.conf"), "path to bridge configuration file (will be created if not present)")
		("timezone,t", po::value<std::string>(), "the timezone to use for new sensors")
		("listen,L", po::value<std::vector<std::string> >(), "listen on this interface and post measurements to mySmartGrid")
		("create,C", po::value<std::string>()->implicit_value(""), "create a configuration and register the device to mySmartGrid")
		("activationcode,A", "print activation code for the mySmartGrid store")
		("heartbeat,H", "perform heartbeat and possibly firmware upgrade");

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).
				options(desc).run(), vm);
		po::notify(vm);

	} catch (const std::exception& e) {
		std::cerr << "Cannot process command line options: " << e.what() << std::endl;
		return ERR_UNKNOWN_PARAMETER;
	}

	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return ERR_NONE;
	}

	// TODO: Compile flag etc.
	if (vm.count("version")) {
		klio::VersionInfo vi;
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		std::cout << "klio library version " << vi.getVersion() << std::endl;
		return ERR_NONE;
	}

	if (!vm.count("config")) {
		std::cerr << "You must specify a configuration file for the bridge" << std::endl;
		return ERR_PARAMETER_MISSING;
	}



	std::string config = vm["config"].as<std::string>();
	if (vm.count("create")) {
		if (vm["create"].as<std::string>() != "") {
			return create_new_store(config, vm["create"].as<std::string>());
		} else {
			return create_new_store(config, "https://api.mysmartgrid.de:8443");
		}
	} else {
		klio::StoreFactory store_factory;
		boost::optional<BridgeConfiguration> parsed_config;

		try {
			if (!exists(boost::filesystem::path(config))) {
				std::cerr << "Configuration file not found" << std::endl;
				return ERR_PARAMETER_VALUE_INVALID;
			} else {
				parsed_config = load_config(config);
			}
		} catch (const std::exception& e) {
			std::cerr << "Could not initialize MSG store: " << e.what() << std::endl;
			return ERR_KLIO;
		}

		BridgeConfiguration& store_config = *parsed_config;
		klio::MSGStore::Ptr store = store_factory.create_msg_store(store_config.url(), store_config.device_id(), store_config.device_key(), STORE_DESCRIPTION, STORE_TYPE);

		if (vm.count("activationcode")) {
			std::cout << store->activation_code() << std::endl;
		} else if (vm.count("heartbeat")) {
			execlp(
					"hexabus_msg_heartbeat",
					"hexabus_msg_heartbeat",
					"--url", store_config.url().c_str(),
					"--device", store_config.device_id().c_str(),
					"--key", store_config.device_key().c_str(),
					(char*) NULL);
			std::cerr << "Could not perform heartbeat: " << strerror(errno) << std::endl;
			return ERR_OTHER;
		} else if (vm.count("listen")) {
			std::string timezone;
			if (!vm.count("timezone")) {
				// TODO: timezone from locale?
				timezone = "Europe/Berlin";
				std::cerr << "Using default timezone " << timezone
					<< ", change with -t <NEW_TIMEZONE>" << std::endl;
			} else {
				timezone = vm["timezone"].as<std::string>();
			}

			try {
				boost::asio::io_service io;
				hexabus::Listener listener(io);
				hexabus::Socket socket(io);

				const std::vector<std::string>& ifaces = vm["listen"].as<std::vector<std::string> >();
				for (std::vector<std::string>::const_iterator it = ifaces.begin(), end = ifaces.end();
						it != end;
						it++) {
					listener.listen(*it);
				}

				store->initialize();

				klio::SensorFactory::Ptr sensor_factory(new klio::SensorFactory());
				klio::TimeConverter::Ptr tc(new klio::TimeConverter());

				hexabus::DeviceInterrogator interrogator(socket);
				hexabus::EndpointRegistry registry;
				ReadingLogger logger(io, *tc, *sensor_factory, timezone, interrogator, registry, store);

				listener.onPacketReceived(boost::ref(logger));

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
		} else {
			std::cerr << "No action specified" << std::endl;
			return ERR_PARAMETER_MISSING;
		}
	}
}
