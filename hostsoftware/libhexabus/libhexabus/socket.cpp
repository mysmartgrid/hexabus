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
	socket(io_service),
	_association_gc_timer(io)
{
  openSocket(&interface);
}

Socket::Socket(boost::asio::io_service& io) :
	io_service(io),
	socket(io_service),
	_association_gc_timer(io)
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
		socket.cancel();
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
	if (error.value() != boost::system::errc::operation_canceled) {
		packetReceivedHandler(error, size);
		beginReceivePacket(false);
	}
}

void Socket::asyncPacketReceivedHandler(const boost::system::error_code& error, size_t size)
{
	if (error.value() != boost::system::errc::operation_canceled) {
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

uint16_t Socket::send(const Packet& packet, const boost::asio::ip::udp::endpoint& dest)
{
	boost::system::error_code err;

	uint16_t seqNum = generateSequenceNumber(dest);

	std::vector<char> data = serialize(packet, seqNum);

	socket.send_to(boost::asio::buffer(&data[0], data.size()), dest, 0, err);
	if (err)
		throw NetworkException("send", err);

	return seqNum;
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

	scheduleAssociationGC();
}

void Socket::scheduleAssociationGC()
{
	_association_gc_timer.expires_from_now(boost::posix_time::hours(1));
}

void Socket::associationGCTimeout(const boost::system::error_code& error)
{
	if (!error) {
		typedef std::map<boost::asio::ip::udp::endpoint, Association> map_t;

		map_t new_associations;
		boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
		for (map_t::const_iterator it = _associations.begin(), end = _associations.end(); it != end; ++it) {
			if (now - it->second.lastUpdate <= boost::posix_time::hours(2)) {
				new_associations.insert(*it);
			}
		}

		_associations = new_associations;
	}

	scheduleAssociationGC();
}

uint16_t Socket::generateSequenceNumber(const boost::asio::ip::udp::endpoint& target)
{
	bool is_new = !_associations.count(target);
	Association& assoc = _associations[target];

	if (is_new) {
		assoc.lastUpdate = boost::posix_time::microsec_clock::universal_time();
		assoc.seqNum = assoc.lastUpdate.time_of_day().total_microseconds() % std::numeric_limits<uint16_t>::max();

		boost::asio::ip::address_v6::bytes_type bytes = target.address().to_v6().to_bytes();
		for (uint32_t i = 0; i < 16; i += 2) {
			assoc.seqNum += bytes[i] | (bytes[i + 1] << 8);
		}

		assoc.seqNum += std::rand() % std::numeric_limits<uint16_t>::max();
	} else {
		assoc.lastUpdate = boost::posix_time::second_clock::universal_time();
	}

	return assoc.seqNum++;
}
