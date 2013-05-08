#include "socket.hpp"

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

const boost::asio::ip::address_v6 Socket::GroupAddress = boost::asio::ip::address_v6::from_string(HXB_GROUP);

Socket::Socket(boost::asio::io_service& io, const std::string& interface) :
  io_service(io),
  socket(io_service)
{
  openSocket(&interface);
}

Socket::Socket(boost::asio::io_service& io) :
  io_service(io),
  socket(io_service)
{
  openSocket(NULL);
}

Socket::~Socket()
{
	boost::system::error_code err;

	socket.close(err);
	// FIXME: maybe errors should be logged somewhere
}

static void timeout_handler(const boost::system::error_code& error, boost::asio::io_service& io)
{
	if (!error) {
		io.stop();
	}
}

void Socket::beginReceivePacket(bool async)
{
	if (!packetReceived.empty()) {
		socket.async_receive_from(boost::asio::buffer(data, data.size()), remoteEndpoint,
				boost::bind(async ? &Socket::asyncPacketReceivedHandler : &Socket::syncPacketReceivedHandler,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
	}
}

void Socket::packetReceivedHandler(const boost::system::error_code& error, size_t size)
{
	if (error)
		throw NetworkException("receive", error);
	Packet::Ptr packet = parseReceivedPacket(size);

	packetReceived(packet, remoteEndpoint);
}

void Socket::syncPacketReceivedHandler(const boost::system::error_code& error, size_t size)
{
	packetReceivedHandler(error, size);
	beginReceivePacket(false);
}

void Socket::asyncPacketReceivedHandler(const boost::system::error_code& error, size_t size)
{
	try {
		packetReceivedHandler(error, size);
	} catch (const GenericException& ge) {
		asyncError(ge);
	} catch (...) {
		beginReceivePacket(true);
		throw;
	}
	beginReceivePacket(true);
}

void Socket::syncPacketReceiveCallback(const Packet::Ptr& packet, const boost::asio::ip::udp::endpoint& from,
		std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint>& result, const filter_t& filter)
{
	if (filter(*packet, from)) {
		result = std::make_pair(packet, from);
		ioService().stop();
	}
}

std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint> Socket::receive(const filter_t& filter, boost::posix_time::time_duration timeout)
{
	std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint> result;

	boost::asio::deadline_timer timer(ioService());
	if (!timeout.is_pos_infinity()) {
		timer.expires_from_now(timeout);
		timer.async_wait(
				boost::bind(
					timeout_handler,
					boost::asio::placeholders::error,
					boost::ref(ioService())));
	}

	boost::signals2::scoped_connection sr(
		packetReceived.connect(
			boost::bind(&Socket::syncPacketReceiveCallback, this, _1, _2, boost::ref(result), boost::cref(filter))));

	beginReceivePacket(false);

	ioService().reset();
	ioService().run();

	return result;
}

static void predicated_receive(const Packet::Ptr& packet, const boost::asio::ip::udp::endpoint& from,
		const Socket::on_packet_received_slot_t& slot, const Socket::filter_t& filter)
{
	if (filter(*packet, from))
		slot(*packet, from);
}

bs2::connection Socket::onPacketReceived(const on_packet_received_slot_t& callback, const filter_t& filter)
{
	bs2::connection result = packetReceived.connect(
			boost::bind(predicated_receive, _1, _2, callback, filter));

	beginReceivePacket(true);

	return result;
}

bs2::connection Socket::onAsyncError(const on_async_error_slot_t& callback)
{
	return asyncError.connect(callback);
}

Packet::Ptr Socket::parseReceivedPacket(size_t size)
{
	return deserialize(&data[0], std::min(size, data.size()));
}

void Socket::send(const Packet& packet, const boost::asio::ip::udp::endpoint& dest)
{
	boost::system::error_code err;

	std::vector<char> data = serialize(packet);

	socket.send_to(boost::asio::buffer(&data[0], data.size()), dest, 0, err);
	if (err)
		throw NetworkException("send", err);
}

void Socket::listen(const boost::asio::ip::address_v6& addr) {
  boost::system::error_code err;

  socket.bind(boost::asio::ip::udp::endpoint(addr, HXB_PORT), err);
  if (err)
    throw NetworkException("listen", err);
}

void Socket::bind(const boost::asio::ip::udp::endpoint& ep) {
  boost::system::error_code err;

  socket.bind(ep, err);
  if (err)
    throw NetworkException("bind", err);
}

void Socket::openSocket(const std::string* interface) {
  boost::system::error_code err;

	data.resize(1500);

  socket.open(boost::asio::ip::udp::v6(), err);
  if (err)
    throw NetworkException("open", err);

  socket.set_option(boost::asio::ip::multicast::hops(64), err);
  if (err)
    throw NetworkException("open", err);

  socket.set_option(boost::asio::socket_base::reuse_address(true), err);
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
	socket.set_option(boost::asio::ip::multicast::enable_loopback(true));
  if (err)
    throw NetworkException("open", err);
}
