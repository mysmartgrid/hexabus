#include <iostream>
#include <fstream>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/network.hpp>
#include <libhexabus/tempsensor.hpp>

// libklio includes. TODO: create support for built-system.
#include <libklio/common.hpp>
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

// TODO: Remove this, only for debugging.
using namespace boost::posix_time;
#include <unistd.h>


#include "../../shared/hexabus_packet.h"


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
  hexabus::NetworkAccess network;
  // TODO: Compile flag etc.
  klio::StoreFactory::Ptr store_factory(new klio::StoreFactory()); 
  klio::Store::Ptr store(store_factory->create_sqlite3_store(db));
  std::cout << "opened store: " << store->str() << std::endl;
  klio::SensorFactory::Ptr sensor_factory(new klio::SensorFactory());
  klio::TimeConverter::Ptr tc(new klio::TimeConverter());

  while(true) {
    network.receivePacket(false);
    char* recv_data = network.getData();
    hexabus::PacketHandling phandling(recv_data);

    // Check for info packets.
    if(phandling.getPacketType() == HXB_PTYPE_INFO) { 
      struct hxb_value value = phandling.getValue();
      float reading=0.0;
      std::string sensor_unit;

      //use the use the right datatype for each recieved packet
      switch(phandling.getDatatype()){
        case HXB_DTYPE_BOOL:
        case HXB_DTYPE_UINT8:
          reading = (float)(*(uint8_t*)&value.data);
          break;
        case HXB_DTYPE_UINT32:
          uint32_t v;
          memcpy(&v, &value.data[0], sizeof(uint32_t));  // damit gehts..
          reading = (float) v;
          break;
        //case 4: //date+time packet
        case HXB_DTYPE_FLOAT:
          memcpy(&reading, &value.data[0], sizeof(float));
          break;
        //case 6: //128char string
        case 7:
          reading = (float)(*(uint32_t*)&value.data);
          break;
        default:
          reading = (float)(*(uint32_t*)&value.data);
      }
      // use the correct dataunit for the endpoints
      switch(phandling.getEID()){
        case 1: 
          sensor_unit=std::string("boolean");
          break;
        case 2: 
          sensor_unit=std::string("Watt");
          break;
        case 3:
          sensor_unit=std::string("deg Celsius");
          break;
        case 4:
          sensor_unit=std::string("boolean");
          break;
        case 5:
          sensor_unit=std::string("% r.h.");
          break;
        case 6:
          sensor_unit=std::string("hPa");
          break;
        case 23:
        case 24:
        case 25:
        case 26:
          sensor_unit=std::string("boolean");
          break;
        default:
          sensor_unit=std::string("unknown");
      }

      /* Uncomment this, if you use an old firmware, where temperature is 
       * transmitted as uint32_t instead of float.
      if( (int)phandling.getEID() == 3 ) {
        // Hack for temp reading. TODO: Update to use new packet format.
        reading = (float)(*(uint32_t*)&value.data)/10000;
      }
      */

      /**
       * 1. Create unique ID for each info message and sensor,
       * <ip>+<endpoint>
       */
      std::ostringstream oss;
      oss << network.getSourceIP().to_string();
      oss << "-";
      oss << (int)phandling.getEID();
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
                sensor_id, sensor_unit, sensor_timezone)); 
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
    } else {
      std::cout << "Received some packet." << std::endl;
    } 
  }

}
