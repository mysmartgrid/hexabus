#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <boost/crc.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

#include "../../shared/hexabus_packet.h"

uint16_t crc16(const char* input, unsigned int length)
{
  boost::crc_optimal<16, 0x1021, 0x0000, 0, true, true> crc; //TODO document this.
  crc.process_bytes(input, length);
  return crc.checksum();
}

void receive_packet(boost::asio::io_service* io_service, boost::asio::ip::udp::socket* socket) // Call with socket = NULL to listen on HEXABUS_PORT, or give a socket where it shall listen on (if you have sent a packet before and are expecting a reply on the port from which it was sent)
{
  bool my_socket = false; // stores whether we were handed the socket pointer from outside or if we are using our own
  if(socket == NULL)
  {
    boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::udp::v6(), HEXABUS_PORT);
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
      struct hxb_packet_bool* packet = (struct hxb_packet_bool*)recv_data;
      if(packet->crc != crc16((char*)packet, sizeof(*packet)-2))
        printf("CRC check failed. Packet may be corrupted.\n");
      printf("Type:\t%d\nFlags:\t%d\nVID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet->type, packet->flags, packet->vid, packet->datatype, packet->value, packet->crc);
    } else if(header->type == HXB_PTYPE_ERROR) {
      printf("Error message.\n");
      struct hxb_packet_error* packet = (struct hxb_packet_error*)recv_data;
      if(packet->crc != crc16((char*)packet, sizeof(*packet)-2))
        printf("CRC check failed. Packet may be corrupted.\n");
      printf("Type:\t%d\nFlags:\t%d\nError Code:\t%d\nCRC:\t%d\n", packet->type, packet->flags, packet->errorcode, packet->crc);
    } else {
      printf("Packet type not yet implemented.\n");
      printf("Type:\t%d\nFlags\t%d\nVID:\t%d\n", header->type, header->flags, header->vid);
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
    printf("       hexaswitch send VID value\n\n");
    printf("commands are:\n");
    printf("  set VID value   set VID to VALUE\n");
    printf("  get VID         query the value of VID\n");
    printf("shortcut commands for Hexabus-Socket:\n");
    printf("  on              switch device on (same as set 0 1)\n");
    printf("  off             switch device off (same as set 0 0)\n");
    printf("  status          query on/off status of device (same as get 0)\n");
    printf("  power           get power consumption (same as get 1)\n");
}

hxb_packet_bool build_setvalue_packet(uint8_t vid, uint8_t datatype, uint8_t value, bool broadcast)
{
  struct hxb_packet_bool packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = broadcast ? HXB_PTYPE_INFO : HXB_PTYPE_WRITE;
  packet.flags = 0;
  packet.vid = vid;
  packet.datatype = datatype;
  packet.value = value;
  packet.crc = crc16((char*)&packet, sizeof(packet)-2);
  // for test, output the Hexabus packet
  printf("Type:\t%d\nFlags:\t%d\nVID:\t%d\nData Type:\t%d\nValue:\t%d\nCRC:\t%d\n", packet.type, packet.flags, packet.vid, packet.datatype, packet.value, packet.crc);
  //TODO implement -v

  return packet;
}

hxb_packet_req build_valuerequest_packet(uint8_t vid)
{
  struct hxb_packet_req packet;
  strncpy((char*)&packet.header, HXB_HEADER, 4);
  packet.type = HXB_PTYPE_QUERY;
  packet.flags = 0;
  packet.vid = vid;
  packet.crc = crc16((char*)&packet, sizeof(packet)-2);
  // for test, output the Hexabus packet
  printf("Type:\t%d\nFlags:\t%d\nVID:\t%d\nCRC:\t%d\n", packet.type, packet.flags, packet.vid, packet.crc);
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
  if(!strcmp(argv[2], "on"))            // on: set VID 0 to TRUE
  {
    hxb_packet_bool packet = build_setvalue_packet(0, HXB_DTYPE_BOOL, HXB_TRUE, false);
    send_packet(socket, argv[1], HEXABUS_PORT, (char*)&packet, sizeof(packet));
  }
  else if(!strcmp(argv[2], "off"))      // off: set VID 0 to FALSE
  {
    hxb_packet_bool packet = build_setvalue_packet(0, HXB_DTYPE_BOOL, HXB_FALSE, false);
    send_packet(socket, argv[1], HEXABUS_PORT, (char*)&packet, sizeof(packet));
  }
  else if(!strcmp(argv[2], "status"))   // status: query VID 0
  {
    hxb_packet_req packet = build_valuerequest_packet(0);
    send_packet(socket, argv[1], HEXABUS_PORT, (char*)&packet, sizeof(packet));
    receive_packet(io_service, socket);
  }
  else if(!strcmp(argv[2], "power"))    // power: query VID 1
  {
    hxb_packet_req packet = build_valuerequest_packet(1);
    send_packet(socket, argv[1], HEXABUS_PORT, (char*)&packet, sizeof(packet));
    receive_packet(io_service, socket);
  }
  else if(!strcmp(argv[2], "set"))      // set: set an arbitrary VID
  {
    if(argc == 5)
    {
      uint8_t val = atoi(argv[4]);
      uint8_t vid = atoi(argv[3]);
      hxb_packet_bool packet = build_setvalue_packet(vid, HXB_DTYPE_UINT8, val, false);
      send_packet(socket, argv[1], HEXABUS_PORT, (char*)&packet, sizeof(packet));
    }
    else
    {
      usage();
      exit(1);
    }
  }
  else if(!strcmp(argv[2], "get"))      // get: request the value of an arbitrary VID
  {
    if(argc == 4)
    {
      uint8_t vid = atoi(argv[3]);
      hxb_packet_req packet = build_valuerequest_packet(vid);
      send_packet(socket, argv[1], HEXABUS_PORT, (char*)&packet, sizeof(packet));
      receive_packet(io_service, socket);
    }
    else
    {
      usage();
      exit(1);
    }
  }
  else if(!strcmp(argv[1], "send"))      // send: send a value broadcast
  {
    if(argc == 4)
    {
      uint8_t val = atoi(argv[3]);
      uint8_t vid = atoi(argv[2]);
      hxb_packet_bool packet = build_setvalue_packet(vid, HXB_DTYPE_UINT8, val, true);
      send_packet(socket, (char*)"ff02::1" , HEXABUS_PORT, (char*)&packet, sizeof(packet));
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
