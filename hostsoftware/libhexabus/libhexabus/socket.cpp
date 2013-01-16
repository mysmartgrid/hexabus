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
  openSocket(boost::asio::ip::address_v6::any(), &interface);
}

Socket::Socket(boost::asio::io_service& io) :
  io_service(io),
  socket(io_service)
{
  openSocket(boost::asio::ip::address_v6::any(), NULL);
}

Socket::~Socket()
{
	boost::system::error_code err;

	socket.close(err);
	// FIXME: maybe errors should be logged somewhere
}

void Socket::beginReceive()
{
	socket.async_receive_from(boost::asio::buffer(data, data.size()), remoteEndpoint,
			boost::bind(&Socket::packetReceiveHandler,
				this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred));
}

std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint> Socket::receive(const filter_t& filter)
{
	boost::system::error_code err;

	while (true) {
		socket.receive_from(boost::asio::buffer(data, data.size()), remoteEndpoint, 0, err);
		if (err)
			throw NetworkException("receive", err);

		Packet::Ptr packet = deserialize(&data[0], data.size());
		if (filter(*packet, remoteEndpoint)) {
			return std::make_pair(packet, remoteEndpoint);
		}
	}
}

class PredicatedReceive {
	private:
		Socket::on_packet_received_slot_t _slot;
		Socket::filter_t _filter;

	public:
		PredicatedReceive(Socket::on_packet_received_slot_t slot, Socket::filter_t filter)
			: _slot(slot), _filter(filter)
		{}

		void operator()(const Packet& packet, const boost::asio::ip::udp::endpoint& from)
		{
			if (_filter(packet, from))
				_slot(packet, from);
		}
};

bs2::connection Socket::onPacketReceived(const on_packet_received_slot_t& callback, const filter_t& filter)
{
	bs2::connection result = packetReceived.connect(PredicatedReceive(callback, filter));

	beginReceive();

	return result;
}

bs2::connection Socket::onAsyncError(const on_async_error_slot_t& callback)
{
	return asyncError.connect(callback);
}

void Socket::packetReceiveHandler(const boost::system::error_code& error, size_t size)
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
		return;
	}
	packetReceived(*packet, remoteEndpoint);
	if (!packetReceived.empty())
		beginReceive();
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

void Socket::openSocket(const boost::asio::ip::address_v6& addr, const std::string* interface) {
  boost::system::error_code err;

	data.resize(HXB_MAX_PACKET_SIZE);

  socket.open(boost::asio::ip::udp::v6(), err);
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
	socket.set_option(boost::asio::ip::multicast::enable_loopback(true));
  if (err)
    throw NetworkException("open", err);
}
