#include <iostream>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <time.h>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <libhexabus/network.hpp>
#include <typeinfo>
namespace po = boost::program_options;

#include "../../../shared/hexabus_packet.h"

#pragma GCC diagnostic warning "-Wstrict-aliasing"


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

void print_packet(const char* recv_data) {
  // FIXME: PacketHandling should accept const char*
  hexabus::PacketHandling phandling(const_cast<char*>(recv_data));

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


template<typename Packet>
void send_packet(hexabus::NetworkAccess* net, const std::string& addr, const Packet& packet, bool printResponse = false)
{
  try {
    net->sendPacket(addr, HXB_PORT, reinterpret_cast<const char*>(&packet), sizeof(packet));
  } catch (const hexabus::NetworkException& e) {
    std::cerr << "Could not send packet to " << addr << ": " << e.code().message() << std::endl;
    exit(1);
  }
  if (printResponse) {
    struct {
      boost::asio::ip::address addr;
      hexabus::NetworkAccess* net;

      void operator()(const boost::asio::ip::address_v6& source, const std::vector<char>& data)
      {
        if (source == this->addr) {
          print_packet(&data[0]);
          this->net->stop();
        }
      }
    } receiveCallback = { boost::asio::ip::address::from_string(addr), net };

    boost::signals2::connection c = net->onPacketReceived(receiveCallback);
    net->run();
    c.disconnect();
  }
}

int main(int argc, char** argv) {

  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " IP [additional options] ACTION";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexabus version and exit")
    ("command,c", po::value<std::string>(), "{get|set|epquery|send|listen|on|off|status|power|devinfo}")
    ("ip,i", po::value<std::string>(), "the hostname to connect to")
    ("bind,b", po::value<std::string>(), "local IP address to use")
    ("interface,I", po::value<std::string>(), "the interface to use for outgoing messages")
    ("eid,e", po::value<uint32_t>(), "Endpoint ID (EID)")
    ("datatype,d", po::value<unsigned int>(), "{1: Bool | 2: UInt8 | 3: UInt32 | 4: HexaTime | 5: Float | 6: String}")
    ("value,v", po::value<std::string>(), "Value")
    ("reliable,r", po::bool_switch(), "Reliable initialization of network access (adds delay, only needed for broadcasts)")
    ;
  po::positional_options_description p;
  p.add("ip", 1);
  p.add("command", 1);
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
  boost::asio::ip::address_v6 bind_addr(boost::asio::ip::address_v6::any());

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  if (vm.count("version")) {
    hexabus::VersionInfo versionInfo;
    std::cout << "hexaswitch -- command line hexabus client" << std::endl;
    std::cout << "libhexabus version " << versionInfo.getVersion() << std::endl;
    return 0;
  }

  if (! vm.count("command")) {
    std::cerr << "You must specify a command." << std::endl;
    return 1;
  } else {
    command=(vm["command"].as<std::string>());
  }

  if (vm.count("bind")) {
    bind_addr = boost::asio::ip::address_v6::from_string(vm["bind"].as<std::string>());
    std::cout << "Binding to " << bind_addr << std::endl;
  }

  hexabus::NetworkAccess* network;
  hexabus::NetworkAccess::InitStyle netInit = 
    vm.count("reliable") && vm["reliable"].as<bool>()
    ? hexabus::NetworkAccess::Reliable
    : hexabus::NetworkAccess::Unreliable;

  if (vm.count("interface")) {
    std::string interface=(vm["interface"].as<std::string>());
    std::cout << "Using interface " << interface << std::endl;
    try {
      network=new hexabus::NetworkAccess(bind_addr, interface, netInit);
    } catch (const hexabus::NetworkException& e) {
      std::cerr << "Could not open socket on interface " << interface << ": " << e.code().message() << std::endl;
      return 1;
    }
  } else {
    try {
      network=new hexabus::NetworkAccess(bind_addr, netInit);
    } catch (const hexabus::NetworkException& e) {
      std::cerr << "Could not open socket: " << e.code().message() << std::endl;
      return 1;
    }
  }

  if(boost::iequals(command, std::string("LISTEN")))
  {
    std::cout << "Entering listen mode." << std::endl;

    struct {
      void operator()(const boost::asio::ip::address_v6& source, const std::vector<char>& data)
      {
        std::cout << "Received packet from " << source << std::endl;
        print_packet(const_cast<char*>(&data[0]));
      }
    } receiveCallback;

    struct {
      void operator()(const hexabus::NetworkException& error)
      {
        std::cerr << "Error receiving packet: " << error.code().message() << std::endl;
        exit(1);
      }
    } errorCallback;

    network->onAsyncError(errorCallback);
    network->onPacketReceived(receiveCallback);
  }

  hexabus::Packet::Ptr packetm(new hexabus::Packet()); // the packet making machine

  /*
   * Shorthand convenience commands.
   */

  if(boost::iequals(command, std::string("ON"))) {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command ON needs an IP address").as<std::string>();
    hxb_packet_int8 packet = packetm->write8(1, HXB_DTYPE_BOOL, HXB_TRUE, false);
    send_packet(network, ip, packet);
  }
  else if(boost::iequals(command, std::string("OFF"))) {// off: set EID 0 to FALSE
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command OFF needs an IP address").as<std::string>();
    hxb_packet_int8 packet = packetm->write8(1, HXB_DTYPE_BOOL, HXB_FALSE, false);
    send_packet(network, ip, packet);
  }
  else if(boost::iequals(command, std::string("STATUS"))) { // status: query EID 1
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command STATUS needs an IP address").as<std::string>();
    hxb_packet_query packet = packetm->query(1);
    send_packet(network, ip, packet, true);

  }
  else if(boost::iequals(command, std::string("POWER")))  // power: query EID 2
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command STATUS needs an IP address").as<std::string>();
    hxb_packet_query packet = packetm->query(2);
    send_packet(network, ip, packet, true);
  }

  /*
   * Generic EID commands.
   */

  else if(boost::iequals(command, std::string("SET"))) // set: set an arbitrary EID
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command SET needs an IP address").as<std::string>();
    uint32_t eid = get_mandatory_parameter(vm,
        "eid", "command SET needs an EID and a datatype").as<uint32_t>();
    unsigned int dtype = get_mandatory_parameter(vm,
        "datatype", "command SET needs an EID and a datatype").as<unsigned int>();
    try { // handle errors in value lexical_cast
      switch(dtype)
      {
        case HXB_DTYPE_BOOL:
        case HXB_DTYPE_UINT8:
          {
            struct hxb_packet_int8 packet8;
            uint8_t val8 = (uint8_t)(boost::lexical_cast<unsigned int>(get_mandatory_parameter(vm,
                  "value", "command SET needs a value").as<std::string>())); // cast to unsigned int first, so that lexical_cast accepts numbers. uint8_t is defined as unsigned char, meaning lexical_cast expects one character as a value as opposed to a number.
            packet8 = packetm->write8(eid, dtype, val8, false);
            std::cout << "Sending value " << (unsigned int)val8 << std::endl;
            send_packet(network, ip, packet8);
          }
          break;
        case HXB_DTYPE_UINT32:
          {
            struct hxb_packet_int32 packet32;
            uint32_t val32 = boost::lexical_cast<uint32_t>(get_mandatory_parameter(vm, "value", "command SET needs a value").as<std::string>());
            packet32 = packetm->write32(eid, dtype, val32, false);
            std::cout << "Sending value " << val32 << std::endl;
            send_packet(network, ip, packet32);
          }
          break;
        case HXB_DTYPE_FLOAT:
          {
            struct hxb_packet_float packetf;
            float valf = boost::lexical_cast<float>(get_mandatory_parameter(vm,
                  "value", "command SET needs a value").as<std::string>());
            packetf = packetm->writef(eid, dtype, valf, false);
            std::cout << "Sending value " << valf << std::endl;
            send_packet(network, ip, packetf);
          }
          break;
        case HXB_DTYPE_128STRING:
          {
            struct hxb_packet_128string packetstr;
            std::string value(get_mandatory_parameter(vm,
                  "value", "command SET needs a value").as<std::string>());
            packetstr = packetm->writestr(eid, dtype, value, false);
            std::cout << "Sending value \"" << value << "\"" << std::endl;
            send_packet(network, ip, packetstr);
          }
          break;
        default:
          std::cout << "unknown data type " << dtype << std::endl;
      }
    } catch (boost::bad_lexical_cast& e) {
      std::cerr << "Error while converting value: " << e.what() << std::endl;
    }
  }

  else if(boost::iequals(command, std::string("GET"))) // get: request the value of an arbitrary EID
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command STATUS needs an IP address").as<std::string>();
    uint32_t eid = get_mandatory_parameter(vm,
          "eid", "command GET needs an EID").as<uint32_t>();
    hxb_packet_query packet = packetm->query(eid);
    send_packet(network, ip, packet, true);
  }

  else if (boost::iequals(command, std::string("EPQUERY")))   // epquery: request endpoint metadata
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command STATUS needs an IP address").as<std::string>();
    uint32_t eid = get_mandatory_parameter(vm,
        "eid", "command GET needs an EID").as<uint32_t>();
    hxb_packet_query packet = packetm->query(eid, true);
    send_packet(network, ip, packet, true);
  }
  else if (boost::iequals(command, std::string("DEVINFO"))) 
    // epquery: request endpoint metadata
  {
    std::string ip = get_mandatory_parameter(vm,
        "ip", "command STATUS needs an IP address").as<std::string>();
    hxb_packet_query packet = packetm->query(0, true);
    send_packet(network, ip, packet, true);
  }
  else if (boost::iequals(command, std::string("SEND")))      // send: send a value broadcast
  {
    uint32_t eid = get_mandatory_parameter(vm,
        "eid", "command SET needs an EID").as<uint32_t>();
    unsigned int dtype = get_mandatory_parameter(vm,
        "datatype", "command SET needs an EID").as<unsigned int>();
    switch(dtype) {
      case HXB_DTYPE_BOOL:
      case HXB_DTYPE_UINT8:
        {
          struct hxb_packet_int8 packet8;
          uint8_t val8 = (uint8_t)boost::lexical_cast<unsigned int>(get_mandatory_parameter(vm,
                  "value", "command SEND needs a value").as<std::string>());
          packet8 = packetm->write8(eid, dtype, val8, true);
          send_packet(network, HXB_GROUP, packet8);
          print_packet((char*)&packet8);
        }
        break;
      case HXB_DTYPE_UINT32:
        {
          struct hxb_packet_int32 packet32;
          uint32_t val32 = boost::lexical_cast<uint32_t>(get_mandatory_parameter(vm,
                  "value", "command SEND needs a value").as<std::string>());
          packet32 = packetm->write32(eid, dtype, val32, true);
          send_packet(network, HXB_GROUP, packet32);
          print_packet((char*)&packet32);
        }
        break;
      case HXB_DTYPE_DATETIME:
        {
          struct hxb_packet_datetime packetdt;
          struct datetime valdt = make_datetime_struct(
              boost::lexical_cast<long>(get_mandatory_parameter(vm,
                  "value", "command SEND needs a value").as<std::string>())
              );
          packetdt = packetm->writedt(eid, dtype, valdt, true);
          send_packet(network, HXB_GROUP, packetdt);
          print_packet((char*)&packetdt);
        }
        break;
      case HXB_DTYPE_FLOAT:
        {
          struct hxb_packet_float packetf;
          float valf = boost::lexical_cast<float>(get_mandatory_parameter(vm,
                  "value", "command SEND needs a value").as<std::string>());
          packetf = packetm->writef(eid, dtype, valf, true);
          send_packet(network, HXB_GROUP, packetf);
          print_packet((char*)&packetf);
        }
        break;
      case HXB_DTYPE_128STRING:
        {
          std::string value(get_mandatory_parameter(vm,
                  "value", "command SEND needs a value").as<std::string>()
              );
          hxb_packet_128string packet = packetm->writestr(eid, dtype, value, true);
          send_packet(network, HXB_GROUP, packet);
          print_packet((char*)&packet);
        }
        break;
      default:
        std::cerr << "Unknown data type for send command." << std::endl;
    }
  } else {
    std::cerr << "Unknown command." << std::endl;
    exit(1);
  }
  exit(0);
}

