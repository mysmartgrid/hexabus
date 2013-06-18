#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include "resolv.hpp"

#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>

#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>

#include <libklio/common.hpp>
#include <libklio/store.hpp>
#include <libklio/store-factory.hpp>
#include <libklio/sensor.hpp>
#include <libklio/sensor-factory.hpp>

namespace po = boost::program_options;

#pragma GCC diagnostic warning "-Wstrict-aliasing"

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

struct ReadingLogger : private hexabus::PacketVisitor {
private:
    boost::asio::ip::address_v6 source;
    float packetValue;
    klio::Store::Ptr store;
    klio::TimeConverter::Ptr tc;
    klio::SensorFactory::Ptr sensor_factory;
    std::string timezone;
    std::map<std::string, time_t> timestamps;
    std::map<std::string, float> readings;
    std::map<std::string, float> measurements;

    const char* eidToUnit(uint32_t eid) {
        switch (eid) {
            case 2:
                return "Watt";
            default:
                return "unknown";
        }
    }

    void rejectPacket() {
        std::cout << "Packet rejected." << std::endl;
    }

    void acceptPacket(float reading, uint32_t eid) {

        try {
            time_t now = tc->get_timestamp();

            //Validate unit
            std::string unit = eidToUnit(eid);
            if (unit.compare("unknown") == 0) {
                //TODO: support other units
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

            if (reading > 0) {
                std::cout << sensor_name << "   " << now << "   " <<
                        "reading: " << reading << " " << unit << std::endl;

                post_watt_reading(sensor, now, reading);
            }

        } catch (klio::StoreException const& ex) {
            std::cout << "Failed to record reading: " << ex.what() << std::endl;

        } catch (std::exception const& ex) {
            std::cout << "Failed to record reading: " << ex.what() << std::endl;
        }
    }

    virtual void post_watt_reading(klio::Sensor::Ptr sensor, time_t now, float reading) {

        time_t previous_timestamp = timestamps[sensor->name()];
        float measurement = 0;

        //If not the first reading
        if (previous_timestamp > 0) {

            float elapsed_time = now - previous_timestamp;
            float previous_reading = readings[sensor->name()];
            measurement = measurements[sensor->name()];

            //Average power
            float power = (reading + previous_reading) / 2;

            //Energy transformed during the elapsed time
            float energy = elapsed_time * power / 3600;

            //Measurement still not posted to MSG
            float pending = (measurement - (long) measurement) + energy;

            measurement += energy;

            if (pending >= 1) {

                //Adjust timestamp for when the accumulated measurement was an integer
                time_t timestamp = now - (long) (elapsed_time * (pending - (long) pending) / pending);

                //Post measurement to MSG
                store->add_reading(sensor, timestamp, (long) measurement);

                std::cout << sensor->name() << "   " << now <<
                        "   posting: /sensor/" << sensor->uuid_short() <<
                        " [" << timestamp << ": " << (long) measurement << "]" << std::endl;
            }
        }

        timestamps[sensor->name()] = now;
        readings[sensor->name()] = reading;
        measurements[sensor->name()] = measurement;
    }

    virtual void visit(const hexabus::ErrorPacket & error) {
        rejectPacket();
    }

    virtual void visit(const hexabus::QueryPacket & query) {
        rejectPacket();
    }

    virtual void visit(const hexabus::EndpointQueryPacket & endpointQuery) {
        rejectPacket();
    }

    virtual void visit(const hexabus::EndpointInfoPacket & endpointInfo) {
        rejectPacket();
    }

    virtual void visit(const hexabus::InfoPacket<bool>& info) {
        acceptPacket(info.value(), info.eid());
    }

    virtual void visit(const hexabus::InfoPacket<uint8_t>& info) {
        acceptPacket(info.value(), info.eid());
    }

    virtual void visit(const hexabus::InfoPacket<uint32_t>& info) {
        acceptPacket(info.value(), info.eid());
    }

    virtual void visit(const hexabus::InfoPacket<float>& info) {
        acceptPacket(info.value(), info.eid());
    }

    virtual void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info) {
        // TODO: handle this properly
    }

    virtual void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info) {
        acceptPacket(info.value().total_seconds(), info.eid());
    }

    virtual void visit(const hexabus::InfoPacket<std::string>& info) {
        // TODO: handle this properly
    }

    virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {
        // TODO: handle this properly
    }

    virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) {
        // TODO: handle this properly
    }

    virtual void visit(const hexabus::WritePacket<bool>& write) {
        rejectPacket();
    }

    virtual void visit(const hexabus::WritePacket<uint8_t>& write) {
        rejectPacket();
    }

    virtual void visit(const hexabus::WritePacket<uint32_t>& write) {
        rejectPacket();
    }

    virtual void visit(const hexabus::WritePacket<float>& write) {
        rejectPacket();
    }

    virtual void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) {
        rejectPacket();
    }

    virtual void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) {
        rejectPacket();
    }

    virtual void visit(const hexabus::WritePacket<std::string>& write) {
        rejectPacket();
    }

    virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {
        rejectPacket();
    }

    virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {
        rejectPacket();
    }

public:

    void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint & from) {
        source = from.address().to_v6();
        visitPacket(packet);
    }

    ReadingLogger(const klio::Store::Ptr& store,
            const klio::TimeConverter::Ptr& tc,
            const klio::SensorFactory::Ptr& sensor_factory,
            const std::string & timezone) :
    store(store), tc(tc), sensor_factory(sensor_factory), timezone(timezone) {
    }
};

int main(int argc, char** argv) {

    std::ostringstream oss;
    oss << "Usage: " << argv[0] << " -I<interface> [-d<device id> -k<device key [-u<store url>]]>]";
    po::options_description desc(oss.str());

    desc.add_options()
            ("help,h", "produce help message")
            ("version,v", "print libklio version and exit")
            ("url,u", po::value<std::string>(), "the remote data store url")
            ("id,d", po::value<std::string>(), "the store id to use")
            ("key,k", po::value<std::string>(), "the store key to use")
            ("timezone,t", po::value<std::string>(), "the timezone to use for new sensors")
            ("interface,I", po::value<std::string>(), "interface to listen on")
            ("bind,b", po::value<std::string>(), "address to bind to");

    po::positional_options_description p;
    p.add("interface", 1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).
                options(desc).positional(p).run(), vm);
        po::notify(vm);

    } catch (const std::exception& e) {
        std::cerr << "Cannot process command line options: " << e.what() << std::endl;
        return ERR_UNKNOWN_PARAMETER;
    }

    // Begin processing of command line parameters.
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

    if ((vm.count("id") && !vm.count("key")) || (!vm.count("id") && vm.count("key"))) {

        std::cerr << "Store id and key are optional arguments, but when informed, must be both defined." << std::endl;
        return ERR_PARAMETER_MISSING;
    }

    if (vm.count("url") && !(vm.count("id") && vm.count("key"))) {

        std::cerr << "When the mySmartGrid API URL is defined, both store id and key are required." << std::endl;
        return ERR_PARAMETER_MISSING;
    }

    if (!vm.count("interface")) {
        std::cerr << "You must specify an interface to listen on." << std::endl;
        return ERR_PARAMETER_MISSING;
    }

    std::string interface(vm["interface"].as<std::string>());
    boost::asio::ip::address_v6 address(boost::asio::ip::address_v6::any());
    boost::asio::io_service io;

    if (vm.count("bind")) {

        boost::system::error_code error;
        address = hexabus::resolve(io, vm["bind"].as<std::string>(), error);
        if (error) {
            std::cerr << vm["bind"].as<std::string>() << " is not a valid IP address: " << error.message() << std::endl;
            return ERR_PARAMETER_FORMAT;
        }
    }

    try {
        klio::StoreFactory::Ptr store_factory(new klio::StoreFactory());
        klio::Store::Ptr store;

        if (!vm.count("url") && vm.count("id")) {

            store = store_factory->create_msg_store(
                    vm["id"].as<std::string>(),
                    vm["key"].as<std::string>());

        } else if (vm.count("url") && vm.count("id")) {

            store = store_factory->create_msg_store(
                    vm["url"].as<std::string>(),
                    vm["id"].as<std::string>(),
                    vm["key"].as<std::string>());
        } else {
            store = store_factory->create_msg_store();
        }
        store->initialize();

        std::string timezone;

        if (!vm.count("timezone")) {
            timezone = "Europe/Berlin";
            std::cerr << "Using default timezone " << timezone
                    << ", change with -t <NEW_TIMEZONE>" << std::endl;
        } else {
            timezone = vm["timezone"].as<std::string>();
        }

        std::cout << std::endl <<
                "Opened store: " << store->str() << std::endl <<
                "Please, go to http://www.mysmartgrid.de/device/mylist and " <<
                "enter the activation code above, in order to link this store to your account." << std::endl <<
                std::endl <<
                "Sensor                        Timestamp    Event" << std::endl <<
                "-------------------------------------------------------------" <<
                std::endl;

        klio::SensorFactory::Ptr sensor_factory(new klio::SensorFactory());
        klio::TimeConverter::Ptr tc(new klio::TimeConverter());

        hexabus::Socket network(io, interface);
        network.listen(address);
        network.onPacketReceived(ReadingLogger(store, tc, sensor_factory, timezone));

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
