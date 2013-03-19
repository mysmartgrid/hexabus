#include "info_query.hpp"
#include <libhexabus/packet.hpp>
#include "../../../../shared/endpoints.h"

using namespace hexanode;

enum ErrorCode {
	ERR_NONE = 0,

	ERR_UNKNOWN_PARAMETER = 1,
	ERR_PARAMETER_MISSING = 2,
	ERR_PARAMETER_FORMAT = 3,
	ERR_PARAMETER_VALUE_INVALID = 4,
	ERR_NETWORK = 5,

	ERR_OTHER = 127
};


template<typename Filter>
ErrorCode send_packet_wait(hexabus::Socket* net, const boost::asio::ip::address_v6& addr, const hexabus::Packet& packet, const Filter& filter)
{
  try {
    net->send(packet, addr);
  } catch (const hexabus::NetworkException& e) {
    std::cerr << "Could not send packet to " << addr << ": " << e.code().message() << std::endl;
  }

	try {
    // TODO: Timeout receive needed.
    //std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> p = 
    //  net->receive(filter && hexabus::filtering::sourceIP() == addr);
    std::cout << "FOOOOOOO" << std::endl;
   // if (*p.first->eid() == 0) {
   //   std::cout << "Device Info" << std::endl
   //     << "Device Name:\t" << endpointInfo.value() << std::endl;
   // }
  } catch (const hexabus::NetworkException& e) {
    std::cerr << "Error receiving packet: " << e.code().message() << std::endl;
    return ERR_NETWORK;
  } catch (const hexabus::GenericException& e) {
    std::cerr << "Error receiving packet: " << e.what() << std::endl;
    return ERR_NETWORK;
  }

  return ERR_NONE;
}

std::string InfoQuery::get_device_name(
          const boost::asio::ip::address_v6& addr)
{
  std::string retval("n/a");
  send_packet_wait(_network, addr, 
      hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR), 
      hexabus::filtering::eid() == EP_DEVICE_DESCRIPTOR);
  return retval;
}
