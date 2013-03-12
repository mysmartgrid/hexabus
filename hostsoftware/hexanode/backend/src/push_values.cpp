#include <libhexanode/common.hpp>
#include <libhexanode/packet_pusher.hpp>
#include <libhexabus/liveness.hpp>
#include <libhexabus/socket.hpp>
#include <iostream>



int main (int argc, char const* argv[]) {
  std::cout << "Pushing incoming values." << std::endl;

  boost::asio::io_service io;
  hexabus::Socket* network;
  std::string interface("eth3");
  network=new hexabus::Socket(io, interface);

  boost::asio::ip::address_v6 bind_addr(boost::asio::ip::address_v6::any());
  std::cout << "Binding to " << bind_addr << std::endl;
  network->listen(bind_addr);

  hexanode::PacketPusher pp(std::cout);
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

    std::cout << "Received packet from " << pair.second << std::endl;
    pp.visitPacket(*pair.first);
  }



  return 0;
}

