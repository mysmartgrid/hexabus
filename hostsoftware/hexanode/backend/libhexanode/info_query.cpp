#include "info_query.hpp"
#include <libhexabus/packet.hpp>
#include "../../../../shared/endpoints.h"

using namespace hexanode;

// TODO: This is copied from the hexinfo.cpp code. 
// Refactor and create a library for this.

struct device_descriptor {
  boost::asio::ip::address_v6 ipv6_address;
  std::string name;

  bool operator<(const device_descriptor &b) const {
    return (ipv6_address < b.ipv6_address);
  }
};

struct InfoCallback { // calback for populating data structures
  device_descriptor* device;
  bool* received;
  void operator()(const hexabus::Packet& packet, 
      const boost::asio::ip::udp::endpoint asio_ep) 
  {
    const hexabus::EndpointInfoPacket* endpointInfo = dynamic_cast<const hexabus::EndpointInfoPacket*>(&packet);
    if(endpointInfo != NULL)
    {
      if(endpointInfo->eid() == EP_DEVICE_DESCRIPTOR) // If it's endpoint 0 (device descriptor), it contains the device name.
      {
        device->name = endpointInfo->value();
        *received = true;
      }
    }
  }
};


struct ErrorCallback {
  void operator()(const hexabus::GenericException& error) {
    std::cerr << "Error receiving packet: " << error.what() << std::endl;
  }
};

std::string InfoQuery::get_device_name(
    const boost::asio::ip::address_v6& target_ip) 
{
  const unsigned int NUM_RETRIES = 5;
  std::string retval("n/a");
  device_descriptor device;
  device.ipv6_address = target_ip;

  std::cout << "Querying device " << target_ip.to_string() << "... ";

  // callback handler for incoming (ep)info packets
  bool received = false;
  InfoCallback infoCallback = { &device, &received };
  ErrorCallback errorCallback;
  boost::signals2::connection c1 = _network->onAsyncError(errorCallback);
  boost::signals2::connection c2 = _network->onPacketReceived(infoCallback,
      (hexabus::filtering::isInfo<uint32_t>() && (hexabus::filtering::eid() == EP_DEVICE_DESCRIPTOR))
      || hexabus::filtering::isEndpointInfo());

  unsigned int retries = 0;

  // epquery the dev.descriptor, to get the name of the device
  while(!received && retries < NUM_RETRIES)
  {
    // send the device name query packet and listen for the reply
    std::cout << retries+1 << " "; 
    _network->send(hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR) , target_ip);

    boost::asio::deadline_timer timer(_network->ioService()); // wait for a second
    timer.expires_from_now(boost::posix_time::milliseconds(1000));
    timer.async_wait(
        boost::bind(&boost::asio::io_service::stop, &_network->ioService()));

    _network->ioService().reset();
    _network->ioService().run();

    retries++;
  }
  std::cout << std::endl;
  if(!received) {
    std::cout << "No reply on device descriptor EPQuery from " << target_ip.to_string() << std::endl;
  } else {
    retval=device.name;
  }

  return retval;
}
