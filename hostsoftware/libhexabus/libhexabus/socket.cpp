#include "socket.hpp"

#include "common.hpp"
#include "packet.hpp"
#include "error.hpp"
#include "private/serialization.hpp"

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <cstring>
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

using namespace hexabus;
namespace bs2 = boost::signals2;

const boost::asio::ip::address_v6 SocketBase::GroupAddress = boost::asio::ip::address_v6::from_string(HXB_GROUP);

bool seqnumIsLessEqual(uint16_t first, uint16_t second) {
	int16_t distance = (int16_t)(first - second);
	return distance <= 0;
}

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
				boost::bind(&SocketBase::packetReceivedHandler,
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

		SocketBase* _this = this;
		BOOST_SCOPE_EXIT((_this)) {
			_this->beginReceive();
		} BOOST_SCOPE_EXIT_END

		try {
			packet = deserialize(&data[0], std::min(size, data.size()));
			if (!packetPrereceive(packet, remoteEndpoint))
				return;
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

	if (filter(*packet, from) && slot != NULL)
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

uint16_t Socket::send(const Packet& packet, const boost::asio::ip::udp::endpoint& dest, uint16_t seqNum)
{
	boost::system::error_code err;

	std::vector<char> data = serialize(packet, seqNum);

	socket.send_to(boost::asio::buffer(&data[0], data.size()), dest, 0, err);
	if (err)
		throw NetworkException("send", err);

	return seqNum;
}

static bool allows_implicit_ack(const Packet &packet) {
	switch(packet.type()) {
		case HXB_PTYPE_QUERY:
		case HXB_PTYPE_EPQUERY:
		case HXB_PTYPE_WRITE:
		case HXB_PTYPE_EP_PROP_WRITE:
		case HXB_PTYPE_EP_PROP_QUERY:
		case HXB_PTYPE_ACK:
			return true;
		default:
			return false;
	}
}

bool Socket::packetPrereceive(const Packet::Ptr& packet, const boost::asio::ip::udp::endpoint& from) {
	Association& assoc = getAssociation(from);

	if((packet->flags()&Packet::want_ack) && !allows_implicit_ack(*packet)) {
		send(AckPacket(packet->sequenceNumber()), from);
	}

	if(seqnumIsLessEqual(packet->sequenceNumber(), assoc.rSeqNum) && assoc.rSeqNum!=0 && !allows_implicit_ack(*packet)) {
		return false;
	} else {

		assoc.rSeqNum = packet->sequenceNumber();
		const CausedPacket* cpacket = dynamic_cast<const CausedPacket*>(&(*packet));

		if(assoc.want_ack_for && ackfilter(*packet, from) && cpacket) {

			if(cpacket->cause() == assoc.want_ack_for) {
				assoc.want_ack_for = 0;
				assoc.retrans_count = 0;
				assoc.currentPacket.first.reset();
				assoc.timeout_timer.cancel();
				assoc.currentPacket.second(*packet, from, false);
				beginSend(from);
			}
		}

		return !filtering::isAck()(*packet, from);
	}
}

void Socket::filterAckReplys(const filter_t& filter)
{
	ackfilter = filter;
}

void Socket::onSendTimeout(const boost::system::error_code& error, const boost::asio::ip::udp::endpoint& to)
{
	if(!error) {
		Association& assoc = getAssociation(to);

		if(assoc.retrans_count >= retrans_limit) {
			assoc.currentPacket.second(*assoc.currentPacket.first, to, true);
			assoc.currentPacket.first.reset();
			beginSend(to);
		} else if(assoc.currentPacket.first.use_count()) {
			assoc.retrans_count++;
			send(*assoc.currentPacket.first, to, assoc.want_ack_for);
			assoc.timeout_timer.expires_from_now(boost::posix_time::milliseconds(500*assoc.retrans_count));
			assoc.timeout_timer.async_wait(boost::bind(&Socket::onSendTimeout, this, _1, to));
		}
	}
}

void Socket::beginSend(const boost::asio::ip::udp::endpoint& to)
{
	Association& assoc = getAssociation(to);

	if(!assoc.currentPacket.first.use_count()) {
		if(!assoc.sendQueue.empty()) {
			assoc.currentPacket.first = assoc.sendQueue.front().first;
			assoc.currentPacket.second = assoc.sendQueue.front().second;
			assoc.want_ack_for = generateSequenceNumber(to);
			assoc.sendQueue.pop();
			send(*assoc.currentPacket.first, to, assoc.want_ack_for);
			assoc.timeout_timer.expires_from_now(boost::posix_time::seconds(1));
			assoc.timeout_timer.async_wait(boost::bind(&Socket::onSendTimeout, this, _1, to));
			beginReceive();
		}
	}
}

void Socket::onPacketTransmitted(
		const on_packet_transmitted_callback_t& callback,
		const Packet& packet,
		const boost::asio::ip::udp::endpoint& dest)
{
	if((packet.flags()&Packet::want_ack)) {
		Association& assoc = getAssociation(dest);
		assoc.sendQueue.push(std::make_pair(packet.clone(), callback));
		beginSend(dest);
	} else {
		send(packet, dest);
		callback(packet, dest, false);
	}
}

void Socket::scheduleAssociationGC()
{
	_association_gc_timer.expires_from_now(boost::posix_time::hours(1));
}

void Socket::associationGCTimeout(const boost::system::error_code& error)
{
	if (!error) {
		typedef std::map<boost::asio::ip::udp::endpoint, boost::shared_ptr<Association> > map_t;

		map_t new_associations;
		boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
		for (map_t::const_iterator it = _associations.begin(), end = _associations.end(); it != end; ++it) {
			if (now - it->second->lastUpdate <= boost::posix_time::hours(2)) {
				new_associations.insert(*it);
			}
		}

		_associations = new_associations;
	}

	scheduleAssociationGC();
}

Socket::Association& Socket::getAssociation(const boost::asio::ip::udp::endpoint& target)
{
	bool is_new = !_associations.count(target);

	if (is_new) {

		_associations[target].reset(new Association(io));
		boost::shared_ptr<Association> assoc = _associations[target];

		//TODO to constructor
		assoc->lastUpdate = boost::posix_time::microsec_clock::universal_time();
		assoc->seqNum = assoc->lastUpdate.time_of_day().total_microseconds() % std::numeric_limits<uint16_t>::max();
		assoc->retrans_count = 0;
		assoc->want_ack_for = 0;
		assoc->rSeqNum = 0;

		boost::asio::ip::address_v6::bytes_type bytes = target.address().to_v6().to_bytes();
		for (uint32_t i = 0; i < 16; i += 2) {
			assoc->seqNum += bytes[i] | (bytes[i + 1] << 8);
		}

		do {
			assoc->seqNum += std::rand() % std::numeric_limits<uint16_t>::max();
		} while(assoc->seqNum == 0);
	}

	return *_associations[target];
}

uint16_t Socket::generateSequenceNumber(const boost::asio::ip::udp::endpoint& target)
{
	Association& assoc = getAssociation(target);
	assoc.lastUpdate = boost::posix_time::second_clock::universal_time();

	return (++assoc.seqNum)==0 ? assoc.seqNum++ : assoc.seqNum;
}
