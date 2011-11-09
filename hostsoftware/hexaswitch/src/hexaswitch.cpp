#include <iostream>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/network.hpp>

#include "../../shared/hexabus_packet.h"

void usage()
{
    std::cout << "\nusage: hexaswitch hostname command\n";
    std::cout << "       hexaswitch listen\n";
    std::cout << "       hexaswitch send EID value\n";
    std::cout << "\ncommands are:\n";
    std::cout << "  set EID datatype value    set EID to VALUE\n";
    std::cout << "  get EID                   query the value of EID\n";
    std::cout << "\nshortcut commands           for Hexabus-Socket:\n";
    std::cout << "  on                        switch device on (same as set 1 1 1)\n";
    std::cout << "  off                       switch device off (same as set 1 1 0)\n";
    std::cout << "  status                    query on/off status of device (same as get 1)\n";
    std::cout << "  power                     get power consumption (same as get 2)\n";
    std::cout << "\ndatatypes are:\n";
    std::cout << "  1                         Bool (Value = 0 or 1)\n";
    std::cout << "  2                         8bit Uint\n";
    std::cout << "  3                         32bit Uint" << std::endl;
}

int main(int argc, char** argv)
{
  hexabus::VersionInfo versionInfo;
  std::cout << "hexaswitch -- command line hexabus client\nlibhexabus version " << versionInfo.getVersion() << std::endl;

  if(argc < 2)
  {
    usage();
    exit(1);
  }

  hexabus::NetworkAccess network;

  if(argc == 2)
  {
    if(!strcmp(argv[1], "listen"))
    {
      while(true)
      {
        network.receivePacket(false);
      }
    }
    else
    {
      usage();
      exit(1);
    }
  }

  hexabus::Packet::Ptr packetm(new hexabus::Packet()); // the packet making machine

  // build the hexabus packet
  if(!strcmp(argv[2], "on"))            // on: set EID 1 to TRUE
  {
    hxb_packet_int8 packet = packetm->write8(1, HXB_DTYPE_BOOL, HXB_TRUE, false);
    network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
  }
  else if(!strcmp(argv[2], "off"))      // off: set EID 0 to FALSE
  {
    hxb_packet_int8 packet = packetm->write8(1, HXB_DTYPE_BOOL, HXB_FALSE, false);
    network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
  }
  else if(!strcmp(argv[2], "status"))   // status: query EID 1
  {
    hxb_packet_query packet = packetm->query(1);
    network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
    network.receivePacket(true);
  }
  else if(!strcmp(argv[2], "power"))    // power: query EID 2
  {
    hxb_packet_query packet = packetm->query(2);
    network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
    network.receivePacket(true);
  }
  else if(!strcmp(argv[2], "set"))      // set: set an arbitrary EID
  {
    if(argc == 6)
    {
      uint8_t eid = atoi(argv[3]);
      uint8_t dtype = atoi(argv[4]);
      uint8_t val8;   // only the relevant one is used in the switch.
      uint32_t val32;

      struct hxb_packet_int8 packet8;
      struct hxb_packet_int32 packet32;

      switch(dtype)
      {
        case HXB_DTYPE_BOOL:
        case HXB_DTYPE_UINT8:
          val8 = atoi(argv[5]);
          packet8 = packetm->write8(eid, dtype, val8, false);
          network.sendPacket(argv[1], HXB_PORT, (char*)&packet8, sizeof(packet8));
          break;
        case HXB_DTYPE_UINT32:
          val32 = atoi(argv[5]);
          packet32 = packetm->write32(eid, dtype, val32, false);
          network.sendPacket(argv[1], HXB_PORT, (char*)&packet32, sizeof(packet32));
          break;
        default:
          std::cout << "unknown data type.\n";
      }
    }
    else
    {
      usage();
      exit(1);
    }
  }
  else if(!strcmp(argv[2], "get"))      // get: request the value of an arbitrary EID
  {
    if(argc == 4)
    {
      uint8_t eid = atoi(argv[3]);
      hxb_packet_query packet = packetm->query(eid);
      network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
      network.receivePacket(true);
    }
    else
    {
      usage();
      exit(1);
    }
  }
  else if(!strcmp(argv[1], "send"))      // send: send a value broadcast
  {  // TODO allow for diffrent data types
    if(argc == 4)
    {
      uint8_t val = atoi(argv[3]);
      uint8_t eid = atoi(argv[2]);
      hxb_packet_int8 packet = packetm->write8(eid, HXB_DTYPE_UINT8, val, true);
      network.sendPacket((char*)"ff02::1" , HXB_PORT, (char*)&packet, sizeof(packet));
    }
    else
    {
      usage();
      exit(1);
    }
  }
  else
  {
    usage();
    exit(1);
  }

  exit(0);
}
