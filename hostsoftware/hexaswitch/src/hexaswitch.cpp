#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <libhexabus/packet.hpp>

#include "../../shared/hexabus_packet.h"
void receive_packet(boost::asio::io_service* io_service, boost::asio::ip::udp::socket* socket) // Call with socket = NULL to listen on HXB_PORT, or give a socket where it shall listen on (if you have sent a packet before and are expecting a reply on the port from which it was sent)
{
  bool my_socket = false; // stores whether we were handed the socket pointer from outside or if we are using our own
  hexabus::CRC::Ptr crc(new hexabus::CRC());
  if(socket == NULL)
  {
    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v6(), HXB_PORT);
    socket = new boost::asio::ip::udp::socket(*io_service, endpoint);
    my_socket = true;
  }
  printf("waiting for data...\n");
  char recv_data[128];
  boost::asio::ip::udp::endpoint remote_endpoint;
  socket->receive_from(boost::asio::buffer(recv_data, 127), remote_endpoint);
  printf("recieved message from %s.\n", remote_endpoint.address().to_string().c_str());

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
        std::cout << (int)value.int8;
        break;
      case HXB_DTYPE_UINT32:
        std::cout << value.int32;
        break;
      default:
        std::cout << "(unknown)";
        break;
    }
    std::cout << std::endl;
  }

  socket->close();
  if(my_socket)
    delete socket;
}

void send_packet(boost::asio::ip::udp::socket* socket, char* addr, unsigned int port, const char* data, unsigned int length)
{
  boost::asio::ip::udp::endpoint remote_endpoint;

  socket->open(boost::asio::ip::udp::v6());
  // Für zum Broadcasten muss da noch einiges mehr -- siehe http://www.ce.unipr.it/~medici/udpserver.html
  
  remote_endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(addr), port);

  boost::system::error_code error; // TODO error message?

  socket->send_to(boost::asio::buffer(data, length), remote_endpoint, 0, error);
}

void usage()
{
    printf("\nusage: hexaswitch hostname command\n");
    printf("       hexaswitch listen\n");
    printf("       hexaswitch send EID value\n\n");
    printf("commands are:\n");
    printf("  set EID datatype value    set EID to VALUE (datatypes: 1 - Bool (0 or 1), 2 - 8bit Uint, 3 - 32bit Uint)\n");
    printf("  get EID                   query the value of EID\n");
    printf("shortcut commands           for Hexabus-Socket:\n");
    printf("  on                        switch device on (same as set 1 1 1)\n");
    printf("  off                       switch device off (same as set 1 1 0)\n");
    printf("  status                    query on/off status of device (same as get 1)\n");
    printf("  power                     get power consumption (same as get 2)\n");
}

int main(int argc, char** argv)
{
  hexabus::VersionInfo versionInfo;
  std::cout << "hexaswitch -- command line hexabus client\nVersion " << versionInfo.getVersion() << std::endl;

  if(argc < 2)
  {
    usage();
    exit(1);
  }

  // open a socket
  boost::asio::io_service* io_service = new boost::asio::io_service();
  boost::asio::ip::udp::socket* socket = new boost::asio::ip::udp::socket(*io_service);

  if(argc == 2)
  {
    if(!strcmp(argv[1], "listen"))
    {
      while(true)
      {
        receive_packet(io_service, NULL);
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
    send_packet(socket, argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
  }
  else if(!strcmp(argv[2], "off"))      // off: set EID 0 to FALSE
  {
    hxb_packet_int8 packet = packetm->write8(1, HXB_DTYPE_BOOL, HXB_FALSE, false);
    send_packet(socket, argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
  }
  else if(!strcmp(argv[2], "status"))   // status: query EID 1
  {
    hxb_packet_query packet = packetm->query(1);
    send_packet(socket, argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
    receive_packet(io_service, socket);
  }
  else if(!strcmp(argv[2], "power"))    // power: query EID 2
  {
    hxb_packet_query packet = packetm->query(2);
    send_packet(socket, argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
    receive_packet(io_service, socket);
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
          send_packet(socket, argv[1], HXB_PORT, (char*)&packet8, sizeof(packet8));
          break;
        case HXB_DTYPE_UINT32:
          val32 = atoi(argv[5]);
          packet32 = packetm->write32(eid, dtype, val32, false);
          send_packet(socket, argv[1], HXB_PORT, (char*)&packet32, sizeof(packet32));
          break;
        default:
          printf("unknown data type.\n");
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
      send_packet(socket, argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
      receive_packet(io_service, socket);
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
      send_packet(socket, (char*)"ff02::1" , HXB_PORT, (char*)&packet, sizeof(packet));
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

  delete socket;

  exit(0);
}
