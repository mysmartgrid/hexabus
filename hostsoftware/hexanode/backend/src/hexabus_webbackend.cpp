#include <libhexanode/common.hpp>
#include <libhexanode/packet_pusher.hpp>
#include <libhexabus/liveness.hpp>
#include <libhexabus/socket.hpp>
#include <libhexanode/sensor.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>
#include <boost/network/uri/uri_io.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;
#include <iostream>

using namespace boost::network;

bool is_info(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint&)
{
	return packet.type() == HXB_PTYPE_INFO;
}

int main (int argc, char const* argv[]) {
  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " [-u] frontendurl [options]";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexanode version and exit")
    ("frontendurl,u", po::value<std::string>(), "URL of frontend API")
    ("interface,i", po::value<std::string>(), "name of the interface to bind to (e.g. eth0)")
    ;
  po::positional_options_description p;
  p.add("frontendurl", 1);
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

  uri::uri base_uri;

  if (vm.count("frontendurl") != 1) {
    std::cerr << "No frontend URL given - exiting." << std::endl;
    return 1;
  } else {
    base_uri << uri::scheme("http") 
      << uri::host(vm["frontendurl"].as<std::string>())
      << uri::path("/api");
  }

  std::cout << "Using frontend url " << base_uri << std::endl;
  std::cout << "Pushing incoming values." << std::endl;

  // We need two io services - the info_io is used during the device name lookup.
  boost::asio::io_service io;
  boost::asio::io_service info_io;
  hexabus::Socket* network;
  if (vm.count("interface") != 1) {
    network=new hexabus::Socket(io);
    std::cout << "Using all interfaces." << std::endl;
  } else {
    std::string interface(vm["interface"].as<std::string>());
    std::cout << "Using interface " << interface << std::endl;
    network=new hexabus::Socket(io, interface);
  }
  boost::asio::ip::address_v6 bind_addr(boost::asio::ip::address_v6::any());
  std::cout << "Binding to " << bind_addr << std::endl;
  network->listen(bind_addr);

  http::client client;

	hexanode::PacketPusher pp(network, client, base_uri, std::cout);
  
  std::cout << "Will now push values." << std::endl;
  while (true) {
    try {
      std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> pair;
      try {
        pair = network->receive(is_info);
      } catch (const hexabus::GenericException& e) {
        const hexabus::NetworkException* nerror;
        if ((nerror = dynamic_cast<const hexabus::NetworkException*>(&e))) {
          std::cerr << "Error receiving packet: " << nerror->code().message() << std::endl;
        } else {
          std::cerr << "Error receiving packet: " << e.what() << std::endl;
        }
        continue;
      }

      if (pair.first == NULL) {
        std::cerr << "Received invalid packet - please check libhexabus version!" << std::endl;
      } else {
        pp.push(pair.second, *pair.first);
      }

    } catch (const std::exception& e) {
      std::cerr << "Unexcepted condition: " << e.what() << std::endl;
      std::cerr << "Discarding and resuming operation." << std::endl;
    }
  }




  return 0;
}

