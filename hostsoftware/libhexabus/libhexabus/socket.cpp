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

const boost::asio::ip::address_v6 SocketBase::GroupAddress = boost::asio::ip::address_v6::from_string(HXB_GROUP);



SocketBase::SocketBase(boost::asio::io_service& io)
	: io(io),
	socket(io),
	data(1024, 0) // ensure arg1 >= max packet size
{
	openSocket();
}

SocketBase::~SocketBase()
{
	boost::system::error_code err;

	socket.close(err);
	// FIXME: maybe errors should be logged somewhere
}

void SocketBase::openSocket()
{
	boost::system::error_code err;

	socket.open(boost::asio::ip::udp::v6(), err);
	if (err)
		throw NetworkException("open", err);
}

int SocketBase::iface_idx(const std::string& iface)
{
	int if_index = if_nametoindex(iface.c_str());
	if (if_index == 0) {
		throw NetworkException(
				"iface_idx",
				boost::system::error_code(
					boost::system::errc::no_such_device,
					boost::system::generic_category()));
	}

	return if_index;
}

void SocketBase::beginReceive()
{
	if (!packetReceived.empty()) {
		socket.cancel();
		socket.async_receive_from(boost::asio::buffer(data, data.size()), remoteEndpoint,
				boost::bind(&Socket::packetReceivedHandler,
					this,
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
	}
}

void SocketBase::packetReceivedHandler(const boost::system::error_code& error, size_t size)
{
	if (error.value() == boost::system::errc::operation_canceled) {
		return;
	} else if (error) {
		asyncError(NetworkException("receive", error));
	} else {
		Packet::Ptr packet;

		beginReceive();
		try {
			packet = deserialize(&data[0], std::min(size, data.size()));
		} catch (const GenericException& ge) {
			asyncError(ge);
			return;
		}
		packetReceived(packet, remoteEndpoint);
	}
}

static void predicated_receive(const Packet::Ptr& packet, const boost::asio::ip::udp::endpoint& from,
		const Socket::on_packet_received_slot_t& slot, const Socket::filter_t& filter)
{
	if (filter(*packet, from))
		slot(*packet, from);
}

bs2::connection SocketBase::onPacketReceived(
		const on_packet_received_slot_t& callback,
		const filter_t& filter)
{
	bs2::connection result = packetReceived.connect(
			boost::bind(predicated_receive, _1, _2, callback, filter));

	beginReceive();

	return result;
}

bs2::connection SocketBase::onAsyncError(const on_async_error_slot_t& callback)
{
	return asyncError.connect(callback);
}

static void receive_handler(
		const Packet::Ptr& packet,
		const boost::asio::ip::udp::endpoint& remote,
		std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint>& target,
		const SocketBase::filter_t& filter,
		boost::asio::io_service& io)
{
	if (filter(*packet, remote)) {
		target = std::make_pair(packet, remote);
		io.stop();
	}
}

static void timeout_handler(const boost::system::error_code& error, boost::asio::io_service& io)
{
	if (!error) {
		io.stop();
	}
}

static void error_handler(const GenericException& e)
{
	throw e;
}

std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint> SocketBase::receive(
		const filter_t& filter,
		boost::posix_time::time_duration timeout)
{
	std::pair<Packet::Ptr, boost::asio::ip::udp::endpoint> result;

	boost::asio::deadline_timer timer(io);
	if (!timeout.is_pos_infinity()) {
		timer.expires_from_now(timeout);
		timer.async_wait(
				boost::bind(timeout_handler, boost::asio::placeholders::error, boost::ref(io)));
	}

	boost::signals2::scoped_connection rc(
		packetReceived.connect(
			boost::bind(receive_handler, _1, _2, boost::ref(result), boost::cref(filter), boost::ref(io))));
	boost::signals2::scoped_connection ec(
		asyncError.connect(
			boost::bind(error_handler, _1)));

	beginReceive();

	ioService().reset();
	ioService().run();

	return result;
}



void Listener::configureSocket()
{
	boost::system::error_code err;

	socket.set_option(boost::asio::socket_base::reuse_address(true), err);
	if (err)
		throw NetworkException("open", err);

	socket.bind(boost::asio::ip::udp::endpoint(GroupAddress, HXB_PORT), err);
	if (err)
		throw NetworkException("open", err);
}

void Listener::listen(const std::string& dev)
{
	boost::system::error_code err;

	int if_index = dev.size() ? iface_idx(dev) : 0;

	socket.set_option(boost::asio::ip::multicast::join_group(GroupAddress, if_index), err);
	if (err)
		throw NetworkException("listen", err);
}

void Listener::ignore(const std::string& dev)
{
	boost::system::error_code err;

	int if_index = dev.size() ? iface_idx(dev) : 0;

	socket.set_option(boost::asio::ip::multicast::leave_group(GroupAddress), err);
	if (err)
		throw NetworkException("ignore", err);
}



void Socket::configureSocket()
{
	boost::system::error_code err;

	socket.set_option(boost::asio::ip::multicast::hops(64), err);
	if (err)
		throw NetworkException("open", err);

	socket.set_option(boost::asio::ip::multicast::enable_loopback(true));
	if (err)
		throw NetworkException("open", err);
}

void Socket::mcast_from(const std::string& dev)
{
	boost::system::error_code err;

	int if_index = iface_idx(dev);
	socket.set_option(boost::asio::ip::multicast::outbound_interface(if_index), err);
	if (err)
		throw NetworkException("mcast_from", err);
}

void Socket::bind(const boost::asio::ip::udp::endpoint& ep)
{
	boost::system::error_code err;

	socket.bind(ep, err);
	if (err)
		throw NetworkException("bind", err);
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
