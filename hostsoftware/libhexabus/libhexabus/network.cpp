#include "network.hpp"

#include "common.hpp"
#include "packet.hpp"
#include "crc.hpp"
#include "error.hpp"
#include "private/serialization.hpp"

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>

#include <boost/bind.hpp>


using namespace hexabus;
namespace bs2 = boost::signals2;

NetworkAccess::NetworkAccess(const std::string& interface, InitStyle init) :
  io_service(),
  socket(io_service)
{
  openSocket(boost::asio::ip::address_v6::any(), &interface, init);
}

NetworkAccess::NetworkAccess(InitStyle init) :
  io_service(),
  socket(io_service)
{
  openSocket(boost::asio::ip::address_v6::any(), NULL, init);
}

NetworkAccess::NetworkAccess(const boost::asio::ip::address_v6& addr, InitStyle init) :
  io_service(),
  socket(io_service)
{
  openSocket(addr, NULL, init);
}

NetworkAccess::NetworkAccess(const boost::asio::ip::address_v6& addr, const std::string& interface, InitStyle init) :
  io_service(),
  socket(io_service)
{
  openSocket(addr, &interface, init);
}

NetworkAccess::~NetworkAccess()
{
	boost::system::error_code err;

	socket.close(err);
	// FIXME: maybe errors should be logged somewhere
}

void NetworkAccess::run()
{
	io_service.run();
}

void NetworkAccess::stop()
{
	io_service.stop();
}

void NetworkAccess::beginReceive()
{
	socket.async_receive_from(boost::asio::buffer(data, sizeof(data)), remoteEndpoint,
			boost::bind(&NetworkAccess::packetReceiveHandler,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
}

bs2::connection NetworkAccess::onPacketReceived(on_packet_received_slot_t callback)
{
	bs2::connection result = packetReceived.connect(callback);

	beginReceive();

	return result;
}

bs2::connection NetworkAccess::onAsyncError(on_async_error_slot_t callback)
{
	return asyncError.connect(callback);
}

void NetworkAccess::packetReceiveHandler(const boost::system::error_code& error, size_t size)
{
	if (error) {
		asyncError(NetworkException("receive", error));
		return;
	}
	Packet::Ptr packet;
	try {
		packet = deserialize(&data[0], data.size());
	} catch (const BadPacketException& e) {
		asyncError(e);
	}
	packetReceived(remoteEndpoint.address().to_v6(), *packet);
	if (!packetReceived.empty())
		beginReceive();
}

void NetworkAccess::sendPacket(std::string addr, uint16_t port, const Packet& packet) {
  boost::asio::ip::udp::endpoint remote_endpoint;
  boost::system::error_code err;

  boost::asio::ip::address targetIP = boost::asio::ip::address::from_string(addr, err);
  if (err)
    throw NetworkException("send", err);
  remote_endpoint = boost::asio::ip::udp::endpoint(targetIP, port);

  std::vector<char> data = serialize(packet);
  
  socket.send_to(boost::asio::buffer(&data[0], data.size()), remote_endpoint, 0, err);
  if (err)
    throw NetworkException("send", err);
}

void NetworkAccess::openSocket(const boost::asio::ip::address_v6& addr, const std::string* interface, InitStyle init) {
  boost::system::error_code err;

  socket.open(boost::asio::ip::udp::v6(), err);
  if (err)
    throw NetworkException("open", err);

  socket.bind(boost::asio::ip::udp::endpoint(addr, 0), err);
  if (err)
    throw NetworkException("open", err);

  socket.set_option(boost::asio::ip::multicast::hops(64), err);
  if (err)
    throw NetworkException("open", err);

  int if_index = 0;

  if (interface) {
    if_index = if_nametoindex(interface->c_str());
    if (if_index == 0) {
      throw NetworkException("open", boost::system::error_code(boost::system::errc::no_such_device, boost::system::generic_category()));
    }
    socket.set_option(boost::asio::ip::multicast::outbound_interface(if_index), err);
    if (err)
      throw NetworkException("open", err);
  }

  socket.set_option(
    boost::asio::ip::multicast::join_group(
      boost::asio::ip::address_v6::from_string(HXB_GROUP),
      if_index),
    err);
  if (err)
    throw NetworkException("open", err);

  if (init == Reliable) {
    // Send invalid packets and wait for a second to build state on multicast routers on the way
    // Two packets will be sent, with one second delay after each. This ensures that routing state is built
    // on at least two consecutive hops, which should be enough for time being.
    // Ideally, this should be replaced by something less reminiscent of brute force
    for (int i = 0; i < 2; i++) {
			// TODO: reserve an endpoint for this?
      sendPacket(HXB_GROUP, 61616, WritePacket<bool>(0, true));

      timeval timeout = { 1, 0 };
      select(0, 0, 0, 0, &timeout);
    }
  }
}
