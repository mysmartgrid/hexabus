#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <libhexabus/crc.hpp>

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
  socket->receive(boost::asio::buffer(recv_data, 127));
  printf("recieved message.\n");

  struct hxb_packet_header* header = (struct hxb_packet_header*)recv_data;
  
  if(strncmp((char*)header, HXB_HEADER, 4))
  {
    printf("Hexabus header not found.\n");
  } else {
    if(header->type == HXB_PTYPE_INFO)
    {
      printf("Info Message.\n");

      // Declare pointers here - but use only the relevant one in the switch()
      struct hxb_packet_int8* packet8;
      struct hxb_packet_int32* packet32;

      switch(header->datatype)
      {
        case HXB_DTYPE_UNDEFINED:
          printf("Undefined datatype.\n");
          break;
        case HXB_DTYPE_BOOL:
        case HXB_DTYPE_UINT8:
          packet8 = (struct hxb_packet_int8*)recv_data;
          if(packet8->crc != crc->crc16((char*)packet8, sizeof(*packet8)-2))
            printf("CRC check failed. Data may be corrupted.\n");

          printf("Type:\t%d\nFlags:\t%d\nEID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet8->type, packet8->flags, packet8->eid, packet8->datatype, packet8->value, packet8->crc);
          break;
        case HXB_DTYPE_UINT32:
          packet32 = (struct hxb_packet_int32*)recv_data;
          if(packet32->crc != crc->crc16((char*)packet32, sizeof(*packet32)-2))
            printf("CRC check failed. Data may be corrupted.\n");

          printf("Type:\t%d\nFlags:\t%d\nEID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet32->type, packet32->flags, packet32->eid, packet32->datatype, packet32->value, packet32->crc);
          break;
        default:
          printf("hexaswitch: Datatype not implemented yet.\n");
      }
    } else if(header->type == HXB_PTYPE_ERROR) {
      printf("Error message.\n");
      struct hxb_packet_error* packet = (struct hxb_packet_error*)recv_data;
      if(packet->crc != crc->crc16((char*)packet, sizeof(*packet)-2))
        printf("CRC check failed. Packet may be corrupted.\n");
      printf("Type:\t%d\nFlags:\t%d\nError Code:\t%d\nCRC:\t%d\n", packet->type, packet->flags, packet->errorcode, packet->crc);
    } else {
      printf("Packet type not yet implemented.\n");
      printf("Type:\t%d\nFlags\t%d\nEID:\t%d\n", header->type, header->flags, header->eid);
    }

    socket->close();
    if(my_socket)
      delete socket;
  }
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

hxb_packet_int8 build_setvalue_packet8(uint8_t eid, uint8_t datatype, uint8_t value, bool broadcast)
{
  hexabus::CRC::Ptr crc(new hexabus::CRC());
  struct hxb_packet_int8 packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = eid;
  packet.datatype = datatype;
  packet.value = value;
  packet.crc = crc->crc16((char*)&packet, sizeof(packet)-2);
  // for test, output the Hexabus packet
  printf("Type:\t%d\nFlags:\t%d\nEID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet.type, packet.flags, packet.eid, packet.datatype, packet.value, packet.crc);

  return packet;
}

hxb_packet_int32 build_setvalue_packet32(uint8_t eid, uint8_t datatype, uint32_t value, bool broadcast)
{
  hexabus::CRC::Ptr crc(new hexabus::CRC());
  struct hxb_packet_int32 packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.eid = eid;
  packet.datatype = datatype;
  packet.value = value; // TODO Byte order problem here?
  packet.crc = crc->crc16((char*)&packet, sizeof(packet)-2);
  // for test, output the Hexabus packet
  printf("Type:\t%d\nFlags:\t%d\nEID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet.type, packet.flags, packet.eid, packet.datatype, packet.value, packet.crc);

  return packet;
}

hxb_packet_query build_valuerequest_packet(uint8_t eid)
{
  hexabus::CRC::Ptr crc(new hexabus::CRC());
  struct hxb_packet_query packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = HXB_PTYPE_QUERY;
  packet.flags = 0;
  packet.eid = eid;
  packet.crc = crc->crc16((char*)&packet, sizeof(packet)-2);
  // for test, output the Hexabus packet
  printf("Type:\t%d\nFlags:\t%d\nEID:\t%d\nCRC:\t%d\n", packet.type, packet.flags, packet.eid, packet.crc);
  //TODO implement -v

  return packet;
}

int main(int argc, char** argv)
{
  printf("hexaswitch -- command line hexabus client\n");

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

  // build the hexabus packet
  if(!strcmp(argv[2], "on"))            // on: set EID 1 to TRUE
  {
    hxb_packet_int8 packet = build_setvalue_packet8(1, HXB_DTYPE_BOOL, HXB_TRUE, false);
    send_packet(socket, argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
  }
  else if(!strcmp(argv[2], "off"))      // off: set EID 0 to FALSE
  {
    hxb_packet_int8 packet = build_setvalue_packet8(1, HXB_DTYPE_BOOL, HXB_FALSE, false);
    send_packet(socket, argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
  }
  else if(!strcmp(argv[2], "status"))   // status: query EID 1
  {
    hxb_packet_query packet = build_valuerequest_packet(1);
    send_packet(socket, argv[1], HXB_PORT, (char*)&packet, sizeof(packet));
    receive_packet(io_service, socket);
  }
  else if(!strcmp(argv[2], "power"))    // power: query EID 2
  {
    hxb_packet_query packet = build_valuerequest_packet(2);
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
          packet8 = build_setvalue_packet8(eid, dtype, val8, false);
          send_packet(socket, argv[1], HXB_PORT, (char*)&packet8, sizeof(packet8));
          break;
        case HXB_DTYPE_UINT32:
          val32 = atoi(argv[5]);
          packet32 = build_setvalue_packet32(eid, dtype, val32, false);
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
      hxb_packet_query packet = build_valuerequest_packet(eid);
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
      hxb_packet_int8 packet = build_setvalue_packet8(eid, HXB_DTYPE_UINT8, val, true);
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
