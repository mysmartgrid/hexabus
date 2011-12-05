#include <iostream>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <time.h>
#include <libhexabus/packet.hpp>
#include <libhexabus/network.hpp>
#include <time.h>

#include "../../shared/hexabus_packet.h"

void usage()
{
    std::cout << "\nusage: hexaswitch hostname command\n";
    std::cout << "       hexaswitch listen\n";
    std::cout << "       hexaswitch send EID datatype value\n";
    std::cout << "\ncommands are:\n";
    std::cout << "  set EID datatype value    set EID to VALUE\n";
    std::cout << "  get EID                   query the value of EID\n";
    std::cout << "  epquery EID               query Endpoint metadata\n";
    std::cout << "\nshortcut commands           for Hexabus-Socket:\n";
    std::cout << "  devinfo                   get device name (same as epquery 0)\n";
    std::cout << "  on                        switch device on (same as set 1 1 1)\n";
    std::cout << "  off                       switch device off (same as set 1 1 0)\n";
    std::cout << "  status                    query on/off status of device (same as get 1)\n";
    std::cout << "  power                     get power consumption (same as get 2)\n";
    std::cout << "\ndatatypes are:\n";
    std::cout << "1: Bool (Value = 0 or 1) - ";
    std::cout << "2: 8bit Uint - ";
    std::cout << "3: 32bit Uint - ";
    std::cout << "4: Hexabus Date and Time. Value can be Unix time or -1 for current system time - ";
    std::cout << "5: 32bit floating point" << std::endl;
}

datetime make_datetime_struct(time_t given_time = -1) {
    struct datetime value;
    time_t raw_time;
    tm *tm_time;

    if(given_time == -1) {
        time(&raw_time);
    } else {
        raw_time = given_time;
    }

    tm_time = localtime(&raw_time);

    value.hour = (uint8_t) tm_time->tm_hour;
    value.minute = (uint8_t) tm_time->tm_min;
    value.second = (uint8_t) tm_time->tm_sec;
    value.day = (uint8_t) tm_time->tm_mday;
    value.month = (uint8_t) tm_time->tm_mon + 1;
    value.year = (uint16_t) tm_time->tm_year + 1900;
    value.weekday = (uint8_t) tm_time->tm_wday;

    return value;

}

void print_packet(char* recv_data) {
  hexabus::PacketHandling phandling(recv_data);

  std::cout << "Hexabus Packet:\t" << (phandling.getOkay() ? "Yes" : "No") << "\nCRC Okay:\t" << (phandling.getCRCOkay() ? "Yes\n" : "No\n");
  if(phandling.getPacketType() == HXB_PTYPE_ERROR)
  {
    std::cout << "Packet Type:\tError\nError Code:\t";
    switch(phandling.getErrorcode())
    {
      case HXB_ERR_UNKNOWNEID:
        std::cout << "Unknown EID\n";
        break;
      case HXB_ERR_WRITEREADONLY:
        std::cout << "Write for ReadOnly Endpoint\n";
        break;
      case HXB_ERR_CRCFAILED:
        std::cout << "CRC Failed\n";
        break;
      case HXB_ERR_DATATYPE:
        std::cout << "Datatype Mismatch\n";
        break;
      default:
        std::cout << "(unknown)\n";
        break;
    } 
  }
  else if(phandling.getPacketType() == HXB_PTYPE_INFO)
  {
    std::cout << "Datatype:\t";
    switch(phandling.getDatatype())
    {
      case HXB_DTYPE_BOOL:
        std::cout << "Bool\n";
        break;
      case HXB_DTYPE_UINT8:
        std::cout << "Uint8\n";
        break;
      case HXB_DTYPE_UINT32:
        std::cout << "Uint32\n";
        break;
      case HXB_DTYPE_DATETIME:
        std::cout << "Datetime\n";
      break;
      case HXB_DTYPE_FLOAT:
        std::cout << "Float\n";
        break;
      default:
        std::cout << "(unknown)";
        break;
    }
    std::cout << "Endpoint ID:\t" << (int)phandling.getEID() << "\nValue:\t\t";
    struct hxb_value value = phandling.getValue();
    switch(value.datatype)
    {
      case HXB_DTYPE_BOOL:
      case HXB_DTYPE_UINT8:
        std::cout << (int)*(uint8_t*)&value.data;
        break;
      case HXB_DTYPE_UINT32:
        std::cout << *(uint32_t*)&value.data;
        break;
      case HXB_DTYPE_DATETIME:
        {
          struct datetime dt = *(datetime*)&value.data;
          std::cout << (int)dt.day << "." << (int)dt.month << "." << dt.year << " " << (int)dt.hour << ":" << (int)dt.minute << ":" << (int)dt.second << " Weekday: " << (int)dt.weekday;
        }
        break;
      case HXB_DTYPE_FLOAT:
        std::cout << *(float*)&value.data;
        break;
      default:
        std::cout << "(unknown)";
        break;
    }
  }
  else if(phandling.getPacketType() == HXB_PTYPE_EPINFO)
  {
    if(phandling.getEID() == 0)
    {
      std::cout << "Device Info\nDevice Name:\t" << phandling.getString() << "\n";
    } else {
      std::cout << "Endpoint Info\n";
      std::cout << "Endpoint ID:\t" << (int)phandling.getEID() << "\n";
      std::cout << "EP Datatype:\t"; // TODO code duplication -- maybe this should be a function
      switch(phandling.getDatatype())
      {
        case HXB_DTYPE_BOOL:
          std::cout << "Bool\n";
          break;
        case HXB_DTYPE_UINT8:
          std::cout << "Uint8\n";
          break;
        case HXB_DTYPE_UINT32:
          std::cout << "Uint32\n";
          break;
        case HXB_DTYPE_DATETIME:
          std::cout << "Datetime\n";
        break;
        case HXB_DTYPE_FLOAT:
          std::cout << "Float\n";
          break;
        default:
          std::cout << "(unknown)\n";
          break;
      }
      std::cout << "EP Name:\t" << phandling.getString() << "\n";
    }
  }
	std::cout << std::endl;
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
        char* recv_data = network.getData();
        std::cout << "Received packet from " << network.getSourceIP() << std::endl;
        std::cout << network.getSourceIP() << " ";
        print_packet(recv_data);
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
    print_packet(network.getData());
  }
  else if(!strcmp(argv[2], "off"))      // off: set EID 0 to FALSE
  {
    hxb_packet_int8 packet = packetm->write8(1, HXB_DTYPE_BOOL, HXB_FALSE, false);
    network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
    print_packet(network.getData());
  }
  else if(!strcmp(argv[2], "status"))   // status: query EID 1
  {
    hxb_packet_query packet = packetm->query(1);
    network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
    network.receivePacket(true);
    print_packet(network.getData());
  }
  else if(!strcmp(argv[2], "power"))    // power: query EID 2
  {
    hxb_packet_query packet = packetm->query(2);
    network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
    network.receivePacket(true);
    print_packet(network.getData());
  }
  else if(!strcmp(argv[2], "set"))      // set: set an arbitrary EID
  {
    if(argc == 6)
    {
      uint8_t eid = atoi(argv[3]);
      uint8_t dtype = atoi(argv[4]);
      uint8_t val8;   // only the relevant one is used in the switch.
      uint32_t val32;
			float valf;

      struct hxb_packet_int8 packet8;
      struct hxb_packet_int32 packet32;
      struct hxb_packet_float packetf;

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
        case HXB_DTYPE_FLOAT:
          valf = atof(argv[5]);
          packetf = packetm->writef(eid, dtype, valf, false);
          network.sendPacket(argv[1], HXB_PORT, (char*)&packetf, sizeof(packetf));
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
      print_packet(network.getData());
    }
    else
    {
      usage();
      exit(1);
    }
  }
  else if(!strcmp(argv[2], "epquery"))   // epquery: request endpoint metadata
  {
    if(argc == 4)
    {
      uint8_t eid = atoi(argv[3]);
      hxb_packet_query packet = packetm->query(eid, true);
      network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
      network.receivePacket(true);
      print_packet(network.getData());
    }
    else
    {
      usage();
      exit(1);
    }
  }
  else if(!strcmp(argv[2], "devinfo"))   // epquery: request endpoint metadata
  {
    if(argc == 3)
    {
      hxb_packet_query packet = packetm->query(0, true);
      network.sendPacket(argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
      network.receivePacket(true);
      print_packet(network.getData());
    }
    else
    {
      usage();
      exit(1);
    }
  }
  else if(!strcmp(argv[1], "send"))      // send: send a value broadcast
  {
    if(argc == 5)
    {
      uint8_t dtype = atoi(argv[3]);
      uint8_t eid = atoi(argv[2]);
      uint8_t val8;
      uint32_t val32;
      struct datetime valdt;
      float valf;

      struct hxb_packet_int8 packet8;
      struct hxb_packet_int32 packet32;
      struct hxb_packet_datetime packetdt;
      struct hxb_packet_float packetf;

      switch(dtype) {
          case HXB_DTYPE_BOOL:
          case HXB_DTYPE_UINT8:
              val8 = atoi(argv[4]);
              packet8 = packetm->write8(eid, dtype, val8, true);
              network.sendPacket((char*)"ff02::1", HXB_PORT, (char*)&packet8, sizeof(packet8));
              print_packet((char*)&packet8);
              break;
          case HXB_DTYPE_UINT32:
              val32 = atoi(argv[4]);
              packet32 = packetm->write32(eid, dtype, val32, true);
              network.sendPacket((char*)"ff02::1", HXB_PORT, (char*)&packet32, sizeof(packet32));
              print_packet((char*)&packet32);
              break;
          case HXB_DTYPE_DATETIME:
              valdt = make_datetime_struct(atol(argv[4]));
              packetdt = packetm->writedt(eid, dtype, valdt, true);
              network.sendPacket((char*)"ff02::1", HXB_PORT, (char*)&packetdt, sizeof(packetdt));
              print_packet((char*)&packetdt);
              break;
          case HXB_DTYPE_FLOAT:
              valf = atof(argv[4]);
              packetf = packetm->writef(eid, dtype, valf, true);
              network.sendPacket((char*)"ff02::1", HXB_PORT, (char*)&packetf, sizeof(packetf));
              print_packet((char*)&packetf);
              break;
          default:
              std::cout << "Unknown data type.\n";
      }
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

