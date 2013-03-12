#include <libhexanode/common.hpp>
#include <libhexanode/packet_pusher.hpp>
#include <libhexanode/valuebuffer.hpp>
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


int main (int argc, char const* argv[]) {
  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " [-u] frontendurl [options]";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexanode version and exit")
    ("frontendurl,u", po::value<std::string>(), "URL of frontend API")
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

  boost::asio::io_service io;
  hexabus::Socket* network;
  std::string interface("eth3");
  network=new hexabus::Socket(io, interface);

  boost::asio::ip::address_v6 bind_addr(boost::asio::ip::address_v6::any());
  std::cout << "Binding to " << bind_addr << std::endl;
  network->listen(bind_addr);

  http::client client;
  std::map<std::string, hexanode::Sensor::Ptr> sensors;
  double min_value = 15;
  double max_value = 30;
  
  std::cout << "Will now push values." << std::endl;
    while (true) {
    std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> pair;
    try {
      pair = network->receive();
    } catch (const hexabus::GenericException& e) {
      const hexabus::NetworkException* nerror;
      if ((nerror = dynamic_cast<const hexabus::NetworkException*>(&e))) {
        std::cerr << "Error receiving packet: " << nerror->code().message() << std::endl;
      } else {
        std::cerr << "Error receiving packet: " << e.what() << std::endl;
      }
      exit(1);
    }

    hexanode::PacketPusher pp(pair.second, client, base_uri, std::cout);
    std::cout << "Received packet from " << pair.second << std::endl;
    pp.visitPacket(*pair.first);

//        hexanode::Sensor::Ptr sensor=sensors.at(sid);
  //        sensor->post_value(client, base_uri, dist(gen));
  //      std::cout << "Cannot submit sensor values (" << e.what() << ")" << std::endl;
  //      std::cout << "Attempting to re-create sensors." << std::endl;
  //        hexanode::Sensor::Ptr sensor=sensors[sid];
  //        sensor->put(client, base_uri, 0.0); 
  //


  }



  return 0;
}

