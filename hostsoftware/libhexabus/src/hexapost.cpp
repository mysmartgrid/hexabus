#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <unistd.h>

#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>

#include <libklio/common.hpp>
#include <libklio/store.hpp>
#include <libklio/store-factory.hpp>
#include <libklio/sensor.hpp>
#include <libklio/sensor-factory.hpp>

#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>

#include "resolv.hpp"

namespace po = boost::program_options;

#pragma GCC diagnostic warning "-Wstrict-aliasing"

struct ReadingLogger : private hexabus::PacketVisitor {
private:
    klio::Store::Ptr store;
    klio::TimeConverter::Ptr tc;
    klio::SensorFactory::Ptr sensor_factory;
    std::string sensor_timezone;

    boost::asio::ip::address_v6 source;
    float packetValue;

    const char* eidToUnit(uint32_t eid) {
        switch (eid) {
            case 2:
                return "Watt";
            default:
                return "unknown";
        }
    }

    void rejectPacket() {
        std::cout << "Received some packet." << std::endl;
    }

    void acceptPacket(float reading, uint32_t eid) {

        //Create unique ID for each info message and sensor, <ip>+<endpoint>
        std::ostringstream oss;
        oss << source.to_string() << "-" << eid;
        std::string sensor_id(oss.str());
        std::cout << "Received a reading from " << sensor_id << ", value "
                << reading << std::endl;

        try {
            klio::Sensor::Ptr sensor;
            bool valid = false;

            //if sensor exists
            std::vector<klio::Sensor::Ptr> sensors = store->get_sensors_by_name(sensor_id);
            if (sensors.size() > 0) {
                std::vector<klio::Sensor::Ptr>::iterator it = sensors.begin();
                sensor = (*it);
                valid = true;

            } else {

                //if unit is known
                std::string unit = eidToUnit(eid);
                if (unit.compare("unknown") != 0) {
                    //Create a new sensor
                    sensor = sensor_factory->createSensor(sensor_id, unit, sensor_timezone);
                    store->add_sensor(sensor);
                    valid = true;
                    std::cout << "Created new sensor: " << sensor->str() << std::endl;
                }
            }

            //If sensor is valid
            if (valid) {
                //Save the value.
                store->add_reading(sensor, tc->get_timestamp(), reading);
                std::cout << "Added reading to sensor " << sensor->name() << std::endl;
            }

        } catch (klio::StoreException const& ex) {
            std::cout << "Failed to record reading: " << ex.what() << std::endl;

        } catch (std::exception const& ex) {
            std::cout << "Failed to record reading: " << ex.what() << std::endl;
        }
    }

    virtual void visit(const hexabus::ErrorPacket& error) {
        rejectPacket();
    }

    virtual void visit(const hexabus::QueryPacket& query) {
        rejectPacket();
    }

    virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) {
        rejectPacket();
    }

    virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo) {
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

    void operator()(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& from) {
        source = from.address().to_v6();
        visitPacket(packet);
    }

    ReadingLogger(const klio::Store::Ptr& store,
            const klio::TimeConverter::Ptr& tc,
            const klio::SensorFactory::Ptr& sensor_factory,
            const std::string& sensor_timezone) :
    store(store), tc(tc), sensor_factory(sensor_factory), sensor_timezone(sensor_timezone) {
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

int main(int argc, char** argv) {

    std::ostringstream oss;
    oss << "Usage: " << argv[0] << " [-s] <interface> <store url>";
    po::options_description desc(oss.str());

    desc.add_options()
            ("help,h", "produce help message")
            ("version,v", "print libklio version and exit")
            ("storeurl,s", po::value<std::string>(), "the remote data store url")
            ("timezone,t", po::value<std::string>(), "the timezone to use for new sensors")
            ("interface,I", po::value<std::string>(), "interface to listen on")
            ("bind,b", po::value<std::string>(), "address to bind to");

    po::positional_options_description p;
    p.add("interface", 1);
    p.add("storeurl", 1);

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

    if (!vm.count("interface")) {
        std::cerr << "You must specify an interface to listen on." << std::endl;
        return ERR_PARAMETER_MISSING;
    }

    if (!vm.count("storeurl")) {
        std::cerr << "You must specify a remote store to work on." << std::endl;
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

        std::string storeurl(vm["storeurl"].as<std::string>());

        //FIXME: remove this line
        std::string id("d271f4de-36cd-f3d3-00db-3e96755d8736");

        //FIXME: use create_msg_store()
        store = store_factory->create_msg_store(storeurl, id, id);
        store->initialize();

        std::string sensor_timezone("Europe/Berlin");
        if (!vm.count("timezone")) {
            std::cerr << "Using default timezone " << sensor_timezone
                    << ", change with -t <NEW_TIMEZONE>" << std::endl;
        } else {
            sensor_timezone = vm["timezone"].as<std::string>();
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
