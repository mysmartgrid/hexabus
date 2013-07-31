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

#include <libklio/store.hpp>
#include <libklio/store-factory.hpp>
#include <libklio/sensor.hpp>
#include <libklio/sensor-factory.hpp>

#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/endpoint_registry.hpp>

#include <libhexanode/device_interrogator.hpp>

namespace po = boost::program_options;

struct ReadingLogger : private hexabus::PacketVisitor {
	private:
		hexanode::DeviceInterrogator interrogator;
		klio::Store::Ptr store;
		klio::TimeConverter::Ptr tc;
		klio::SensorFactory::Ptr sensor_factory;
		std::string timezone;

		boost::asio::ip::address_v6 source;
		hexabus::EndpointRegistry ep_registry;

		const char* eidToUnit(uint32_t eid) {
			hexabus::EndpointRegistry::const_iterator found = ep_registry.find(eid);

			if (found == ep_registry.end() || !found->second.unit())
				return NULL;

			const std::string& unit = *found->second.unit();

			if (unit == "kWh")
				return "kWh";
			else if (unit == "degC")
				return "degC";
			else if (unit == "%r.h.")
				return "rh";
			else if (unit == "hPa")
				return "hPa";

			return NULL;
    }

		void acceptPacket(float reading, uint32_t eid)
		{
			try {
				time_t now = tc->get_timestamp();

				//Validate unit
				const char* unit = eidToUnit(eid);
				if (!unit) {
					return;
				}

				//Create unique ID for each info message and sensor, <ip>+<endpoint>
				std::ostringstream oss;
				oss << source.to_string() << "-" << eid;
				std::string sensor_name(oss.str());

				std::vector<klio::Sensor::Ptr> sensors = store->get_sensors_by_name(sensor_name);
				klio::Sensor::Ptr sensor;

				//If sensor already exists
				if (sensors.size() > 0) {
					std::vector<klio::Sensor::Ptr>::iterator it = sensors.begin();
					sensor = (*it);

				} else {
					//Create a new sensor
					sensor = sensor_factory->createSensor(sensor_name, unit, timezone);
					store->add_sensor(sensor);
					std::cout << sensor_name << "   " <<
						now << "   created" << std::endl;
				}
				store->add_reading(sensor, now, reading);

			} catch (klio::StoreException const& ex) {
				std::cout << "Failed to record reading: " << ex.what() << std::endl;

			} catch (std::exception const& ex) {
				std::cout << "Failed to record reading: " << ex.what() << std::endl;
			}
		}

		// these packets contain readings. process them.
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

		// don't process these packets, they never contain useful readings
		virtual void visit(const hexabus::ErrorPacket & error) { }
		virtual void visit(const hexabus::QueryPacket & query) { }
		virtual void visit(const hexabus::EndpointQueryPacket & endpointQuery) { }
		virtual void visit(const hexabus::EndpointInfoPacket & endpointInfo) { }
		virtual void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info) { }
		virtual void visit(const hexabus::InfoPacket<std::string>& info) { }
		virtual void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info) { }
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { }
		virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { }
		virtual void visit(const hexabus::WritePacket<bool>& write) { }
		virtual void visit(const hexabus::WritePacket<uint8_t>& write) { }
		virtual void visit(const hexabus::WritePacket<uint32_t>& write) { }
		virtual void visit(const hexabus::WritePacket<float>& write) { }
		virtual void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) { }
		virtual void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) { }
		virtual void visit(const hexabus::WritePacket<std::string>& write) { }
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) { }
		virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) { }

	public:
		ReadingLogger(hexabus::Socket& socket,
				const klio::Store::Ptr& store,
				const klio::TimeConverter::Ptr& tc,
				const klio::SensorFactory::Ptr& sensor_factory,
				const std::string & timezone)
			: interrogator(socket), store(store), tc(tc), sensor_factory(sensor_factory), timezone(timezone)
		{
		}

		void onPacket(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint & from)
		{
			source = from.address().to_v6();
			visitPacket(packet);
		}
};


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

static ErrorCode create_new_store(const std::string&  config, boost::optional<const std::string&> url)
{
	boost::uuids::random_generator new_uuid;
	klio::StoreFactory store_factory;
	klio::MSGStore::Ptr store;

	if (url) {
		store = store_factory.create_msg_store(
				*url,
				to_string(new_uuid()),
				to_string(new_uuid()));
	} else {
		store = store_factory.create_msg_store();
	}
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
	oss << "Usage: " << argv[0] << " -c <configuration file> { -L <interface> | -C [url] | -H }";
	po::options_description desc(oss.str());

	desc.add_options()
		("help,h", "produce help message")
		("version,v", "print version info and exit")
		("config,c", po::value<std::string>(), "path to bridge configuration file (will be created if not present)")
		("timezone,t", po::value<std::string>(), "the timezone to use for new sensors")
		("listen,L", po::value<std::string>(), "listen on interface and post measurements to mySmartGrid")
		("create,C", po::value<std::string>()->implicit_value(""), "create a configuration and register the device to mySmartGrid")
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
			return create_new_store(config, boost::none);
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
		klio::MSGStore::Ptr store = store_factory.create_msg_store(store_config.url(), store_config.device_id(), store_config.device_key());

		if (vm.count("heartbeat")) {
			execlp(
					"hexabus_msg_heartbeat",
					"hexabus_msg_heartbeat",
					"--url", store_config.url().c_str(),
					"--device", store_config.device_id().c_str(),
					"--key", store_config.device_key().c_str(),
					(char*) NULL);
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
				store->initialize();

				klio::SensorFactory::Ptr sensor_factory(new klio::SensorFactory());
				klio::TimeConverter::Ptr tc(new klio::TimeConverter());

				boost::asio::io_service io;
				hexabus::Socket socket(io, vm["listen"].as<std::string>());
				socket.listen(boost::asio::ip::address_v6::any());

				ReadingLogger logger(socket, store, tc, sensor_factory, timezone);

				socket.onPacketReceived(boost::bind(&ReadingLogger::onPacket, &logger, _1, _2));

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
			std::cerr << "You must specify which action I should take" << std::endl;
			return ERR_PARAMETER_MISSING;
		}
	}
}
