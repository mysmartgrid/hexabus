#include <iostream>
#include <fstream>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <time.h>
#include <libhexabus/packet.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <libhexabus/network.hpp>
#include <typeinfo>
namespace po = boost::program_options;

#include "../../../shared/hexabus_packet.h"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

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
      case HXB_DTYPE_128STRING:
        std::cout << "String\n";
        break;
      default:
        std::cout << "(unknown)";
        break;
    }
    std::cout << "Endpoint ID:\t" << phandling.getEID() << std::endl;
    std::cout << "Value:\t\t";
    //struct hxb_value value = phandling.getValue();
    struct hxb_value value;
    phandling.getValuePtr(&value);
    switch(value.datatype)
    {
      case HXB_DTYPE_BOOL:
      case HXB_DTYPE_UINT8:
        std::cout << (int)*(uint8_t*)&value.data;
        break;
      case HXB_DTYPE_UINT32:
        {
        uint32_t v;
        memcpy(&v, &value.data[0], sizeof(uint32_t));  // damit gehts..
        std::cout << v << std::endl;
        }
        break;
      case HXB_DTYPE_DATETIME:
        {
          struct datetime dt = *(datetime*)&value.data;
          std::cout << (int)dt.day << "." << (int)dt.month << "." << dt.year << " " << (int)dt.hour << ":" << (int)dt.minute << ":" << (int)dt.second << " Weekday: " << (int)dt.weekday;
        }
        break;
      case HXB_DTYPE_FLOAT:
        {
          float v;
          memcpy(&v, &value.data[0], sizeof(float));
          std::cout << v;
        }
        break;
      case HXB_DTYPE_128STRING:
        std::cout << phandling.getString();
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
      std::cout << "Endpoint ID:\t" << phandling.getEID() << "\n";
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
        case HXB_DTYPE_128STRING:
          std::cout << "String\n";
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

po::variable_value get_mandatory_parameter(
    po::variables_map vm,
    std::string param_id,
    std::string error_message
    )
{
  po::variable_value retval;
  try {
    if (! vm.count(param_id)) {
      std::cerr << error_message << std::endl;
      exit(-1);
    } else {
      retval=(vm[param_id]);
    }
  } catch (std::exception& e) {
    std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
    exit(-1);
  }
  return retval;
}


int main(int argc, char** argv) {

  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " IP [additional options] ACTION";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexabus version and exit")
    ("ip,i", po::value<std::string>(), "the hostname to connect to")
    ("interface,I", po::value<std::string>(), "the interface to use for outgoing messages")
    ("program,p", po::value<std::string>(), "the state machine program to be uploaded")
    ;
  po::positional_options_description p;
  p.add("ip", 1);
  p.add("program", 1);
  po::variables_map vm;

  // Begin processing of commandline parameters.
  try {
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
    exit(-1);
  }

  std::string command;
  std::string program;

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  if (vm.count("version")) {
    hexabus::VersionInfo versionInfo;
    std::cout << "hexaswitch -- command line hexabus client" << std::endl;
    std::cout << "libhexabus version " << versionInfo.getVersion() << std::endl;
    return 0;
  }

  if (!vm.count("program")) {
    std::cout << "Cannot proceed without a program (-p <FILE>)" << std::endl;
    exit(-1);
  } else {
    std::ifstream in(vm["program"].as<std::string>().c_str(), 
        std::ios_base::in | std::ios::ate | std::ios::binary);
    if (!in) {
      std::cerr << "Error: Could not open input file: "
        << vm["program"].as<std::string>() << std::endl;
      exit(-1);
    } 
    in.unsetf(std::ios::skipws); // No white space skipping!

    size_t size = in.tellg();
    in.seekg(0, std::ios::beg);
    char* buffer = new char[size];
    std::cout << "size " << size << std::endl;
    in.read(buffer, size);
    std::string instr(buffer, size);
    delete buffer;
    in.close();
    program = instr;
  }

  hexabus::NetworkAccess* network;

  if (vm.count("interface")) {
    std::string interface=(vm["interface"].as<std::string>());
    std::cout << "Using interface " << interface << std::endl;
    network=new hexabus::NetworkAccess(interface);
  } else {
    network=new hexabus::NetworkAccess();
  }

  hexabus::Packet::Ptr packetm(new hexabus::Packet()); // the packet making machine

  /**
    for all packets to be send:
    0. Attempt counter: Initialize to 0
    1. generate 66bytes packet
    2. if attempt < max_attempt:
    send it, start timer, wait for response
    else 
    exit with failure note
    3. evaluate answer:
    if ACK: send next packet
else: send current packet again, increase attempt counter
goto 2
   */

  std::string ip_addr(get_mandatory_parameter(vm,
        "ip", "command 'program' needs an IP address").as<std::string>()
      );
  hxb_packet_66bytes packet = packetm->writebytes(10, HXB_DTYPE_66BYTES, program.c_str(), program.length(), false);
  network->sendPacket(ip_addr.c_str(), HXB_PORT, (char*)&packet, sizeof(packet));
  print_packet((char*)&packet);

  exit(0);
}

