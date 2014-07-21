#include <libhexanode/common.hpp>
#include <libhexanode/packet_pusher.hpp>
#include <libhexabus/liveness.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/filtering.hpp>
#include <libhexanode/solar_calculator.hpp>
#include <libhexanode/historian.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>
#include <boost/network/uri/uri_io.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include "../../../shared/endpoints.h"

using namespace boost::network;
namespace hf = hexabus::filtering;


int main (int argc, char const* argv[]) {
  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " [-u] frontendurl [options]";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexanode version and exit")
    ("interface,i", po::value<std::string>(), "name of the interface to bind to (e.g. eth0)")
    ("config,c", po::value<std::string>(), "configuration file to use")
    ;
  po::positional_options_description p;
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

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  if (vm.count("version")) {
    hexanode::VersionInfo v;
    std::cout << "hexanode version " << v.get_version() << std::endl;
    return 0;
  }
  boost::property_tree::ptree config_tree;
  if (vm.count("config") != 1) {
    std::cerr << "You must provide a configuration file (-c <FILE>). Aborting." << std::endl;
    return 1;
  } else {
    std::string conf_file(vm["config"].as<std::string>());
    std::cout << "Attempting to read configuration from file " << conf_file << std::endl;
    boost::property_tree::ini_parser::read_ini(conf_file, config_tree);
  }

  try {
    std::cout << "Using this configuration:" << std::endl;
    std::cout << "* General configuration" << std::endl;
    std::cout << "   Send IPv6: " << config_tree.get<std::string>("general.send_ipv6") << std::endl;
    std::cout << "   Update interval: " << config_tree.get<uint32_t>("general.update_interval") << std::endl;
    std::cout << "* Photovoltaik production" << std::endl;
    //std::cout << "   Dial IPv6: " << config_tree.get<std::string>("photovoltaik.dial_ipv6") << std::endl;
    std::cout << "   Dial EID: " << config_tree.get<std::string>("photovoltaik.dial_eid") << std::endl;
    std::cout << "   Peak Watt: " << config_tree.get<std::string>("photovoltaik.peak_watt") << std::endl;
    std::cout << "* Battery" << std::endl;
    std::cout << "   Dial EID: " << config_tree.get<std::string>("battery.dial_eid") << std::endl;
    std::cout << "   Peak Watt: " << config_tree.get<std::string>("battery.peak_watt") << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Failed to evaluate configuration file - error was: "
      << e.what() << std::endl;
    return 1;
  }

  boost::asio::ip::address_v6 send_ipv6 = 
    boost::asio::ip::address_v6::from_string(
        config_tree.get<std::string>("general.send_ipv6"));
  uint32_t update_interval = config_tree.get<uint32_t>("general.update_interval");
  //boost::asio::ip::address_v6 pv_dial_ipv6 = 
  //  boost::asio::ip::address_v6::from_string(
  //      config_tree.get<std::string>("photovoltaik.dial_ipv6") 
  //      ); 
  uint32_t pv_production_eid = config_tree.get<uint32_t>("photovoltaik.dial_eid");
  uint32_t pv_peak_watt = config_tree.get<uint32_t>("photovoltaik.peak_watt");
  uint32_t battery_eid = config_tree.get<uint32_t>("battery.dial_eid");
  uint32_t battery_peak_watt = config_tree.get<uint32_t>("battery.peak_watt");



  // We need two listen_io services - the send_io is used during the device name lookup.
  boost::asio::io_service listen_io;
  boost::asio::io_service send_io;
  boost::asio::io_service send_io2;
  hexabus::Listener* listen_network;
  hexabus::Socket* send_network;
  hexabus::Socket* send_network2;
  if (vm.count("interface") != 1) {
    listen_network=new hexabus::Listener(listen_io);
    send_network=new hexabus::Socket(send_io);
    send_network2=new hexabus::Socket(send_io2);
    std::cout << "Using all interfaces." << std::endl;
  } else {
    std::string interface(vm["interface"].as<std::string>());
    std::cout << "Using interface " << interface << std::endl;
    listen_network=new hexabus::Listener(listen_io);
		listen_network->listen(interface);
    send_network=new hexabus::Socket(send_io);
		send_network->mcast_from(interface);
    send_network2=new hexabus::Socket(send_io2);
  }
  boost::asio::ip::address_v6 listen_addr(boost::asio::ip::address_v6::any());
  std::cout << "Listening to " << listen_addr << std::endl;
  std::cout << "Sending updates via " << send_ipv6 << std::endl;
  send_network->bind(send_ipv6);
  send_network2->bind(send_ipv6);

  hexanode::Historian::Ptr h(new hexanode::Historian(send_network2, update_interval));
  h->run();

  while (true) {
    try {
      std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> pair;
      try {
        pair = listen_network->receive(
            (hf::eid() == pv_production_eid) 
            | (hf::eid() == battery_eid)
            | (hf::eid() == EP_POWER_METER)
            );
      } catch (const hexabus::GenericException& e) {
        const hexabus::NetworkException* nerror;
        if ((nerror = dynamic_cast<const hexabus::NetworkException*>(&e))) {
          std::cerr << "Error receiving packet: " << nerror->code().message() << std::endl;
        } else {
          std::cerr << "Error receiving packet: " << e.what() << std::endl;
        }
        continue;
      }

      std::cout << "Received update from " << pair.second << std::endl;
      hexanode::SolarCalculator sc(listen_network, send_network, pair.second, 
          pv_production_eid, pv_peak_watt, battery_eid, battery_peak_watt, h);
      if (pair.first == NULL) {
        std::cerr << "Received invalid packet - please check libhexabus version!" << std::endl;
      } else {
        sc.visitPacket(*pair.first);
      }

    } catch (const std::exception& e) {
      std::cerr << "Unexcepted condition: " << e.what() << std::endl;
      std::cerr << "Discarding and resuming operation." << std::endl;
    }
  }

  h->shutdown();

  return 0;
}

