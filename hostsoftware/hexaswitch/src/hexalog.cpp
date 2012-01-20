#include <iostream>
#include <fstream>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/network.hpp>
#include <libhexabus/tempsensor.hpp>


// TODO: Remove this, only for debugging.
using namespace boost::posix_time;
#include <unistd.h>


#include "../../shared/hexabus_packet.h"

void usage()
{
  std::cout << "\nusage: hexaswitch hostname command\n";
  std::cout << "       hexaswitch listen\n";
}


const uint32_t write_interval = 2;

int main(int argc, char** argv)
{
  hexabus::VersionInfo versionInfo;
  std::cout << "hexaswitch -- command line hexabus client" << std::endl 
    << "libhexabus version " << versionInfo.getVersion() << std::endl;

  std::map<std::string, hexabus::Sensor::Ptr> sensors;
  hexabus::NetworkAccess network;

  uint32_t packet_counter=0;
  while(true) {
    network.receivePacket(false);
    char* recv_data = network.getData();
    hexabus::PacketHandling phandling(recv_data);

    // Check for info packets.
    if(phandling.getPacketType() == HXB_PTYPE_INFO) { 
      // Do we have a temperature value?
      std::map<std::string, hexabus::Sensor::Ptr>::iterator it;
      if( (int)phandling.getEID() == 3 ) {
        struct hxb_value value = phandling.getValue();
        // Check if we have seen this sensor previously
        float converted_value = (float)(*(uint32_t*)&value.data)/10000;

        std::string sensor_name(network.getSourceIP().to_string());
        it = sensors.find(sensor_name);

        if (it != sensors.end()) {
          // Yes, its there!
          LOG("Adding to existing sensor " << sensor_name << ", value " << converted_value);
          (*it).second->add_value(converted_value);
        } else {
          LOG("Creating new sensor " << sensor_name << ", value " << converted_value);
          // No, create a new one
          hexabus::Sensor::Ptr sensor(new hexabus::TempSensor(sensor_name));
          sensors.insert( 
              std::pair<std::string, hexabus::Sensor::Ptr>(sensor_name, sensor) 
              );
          sensor->add_value(converted_value);
        }

        packet_counter++;
        // Each write_interval sensor values, save the current sensor states.
        if (packet_counter >= write_interval ) {
          packet_counter=0;
          for ( it=sensors.begin() ; it != sensors.end(); it++ ) {
            std::string name=(*it).first;
            hexabus::Sensor::Ptr current((*it).second);
            // open a file with the sensor's name.
            std::ofstream sensor_file(name.c_str());
            current->save_values(sensor_file);
            sensor_file.close();
          }
        }
      }
    }
  }

}
