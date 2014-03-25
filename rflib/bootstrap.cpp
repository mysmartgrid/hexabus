#include "bootstrap.hpp"

#include "nlmessages.hpp"
#include "nlparsers.hpp"

#include "boost/tuple/tuple.hpp"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <stdexcept>

BootstrapSocket::BootstrapSocket(const std::string& iface, bool nosec)
	: _fd(socket(AF_IEEE802154, SOCK_DGRAM | SOCK_CLOEXEC, 0)),
	  _iface(iface)
{
	if (_fd < 0)
		throw std::runtime_error(strerror(errno));

	if (nosec) {
		int val = SO_802154_SECURITY_OFF;

		int rc = setsockopt(_fd, SOL_IEEE802154, SO_802154_SECURITY, &val, sizeof(val));
		if (rc < 0)
			throw std::runtime_error(strerror(errno));
	}
}

std::pair<std::vector<uint8_t>, sockaddr_ieee802154> BootstrapSocket::receive()
{
	std::vector<uint8_t> packet(128, 0);
	sockaddr_ieee802154 peer;
	socklen_t peerlen = sizeof(peer);
	sockaddr* addr = reinterpret_cast<sockaddr*>(&peer);

	int len = recvfrom(_fd, &packet[0], packet.size(), 0, addr, &peerlen);
	if (len < 0)
		throw std::runtime_error(strerror(errno));

	packet.resize(len);
	return std::make_pair(packet, peer);
}

void BootstrapSocket::send(const void* msg, size_t len, const sockaddr_ieee802154& peer)
{
	socklen_t peerlen = sizeof(peer);
	const sockaddr* addr = reinterpret_cast<const sockaddr*>(&peer);

	ssize_t sent = sendto(_fd, msg, len, 0, addr, peerlen);
	if (sent < 0)
		throw std::runtime_error(strerror(errno));
}



PairingHandler::PairingHandler(const std::string& iface, uint16_t pan_id)
	: BootstrapSocket(iface, true), _pan_id(pan_id)
{
}

bool key_invalid(const Key& key)
{
	return key.lookup_desc().mode() != 1;
}

int key_compare(const Key& a, const Key& b)
{
	return a.lookup_desc().id() - b.lookup_desc().id();
}

void PairingHandler::run_once()
{
	struct {
		uint8_t msg;
		uint16_t pan_id;
		uint8_t key[16];
	} __attribute__((packed)) packet;

	std::vector<uint8_t> recv_data;
	sockaddr_ieee802154 peer;

	boost::tie(recv_data, peer) = receive();
	if (!recv_data.size() || recv_data[0] != HXB_B_PAIR_REQUEST)
		return;

	if (recv_data.size() != 1)
		throw std::runtime_error("invalid PAIR_REQUEST");

	packet.msg = HXB_B_PAIR_RESPONSE;
	packet.pan_id = htons(_pan_id);

	Key key = control().get_out_key(iface());
	memcpy(packet.key, key.key(), 16);

	uint64_t addr;
	memcpy(&addr, peer.addr.hwaddr, 8);

	try {
		Device dev(iface(), _pan_id, 0xfffe, addr, 0, key.lookup_desc());

		control().add_device(dev);
	} catch (const nl::nl_error& e) {
		if (e.error() != NLE_EXIST)
			throw;
	}

	send(&packet, sizeof(packet), peer);
}



void ResyncHandler::run_once()
{
	struct {
		uint8_t msg;
		uint32_t ctr;
	} __attribute__((packed)) packet;

	std::vector<uint8_t> recv_data;
	sockaddr_ieee802154 peer;

	boost::tie(recv_data, peer) = receive();
	if (!recv_data.size() || recv_data[0] != HXB_B_RESYNC_REQUEST)
		return;

	if (recv_data.size() != 1)
		throw std::runtime_error("invalid RESYNC_REQUEST");

	packet.msg = HXB_B_RESYNC_RESPONSE;

	uint64_t addr;

	memcpy(&addr, peer.addr.hwaddr, 8);
	Device dev = control().list_devices(iface(), addr).at(0);

	packet.ctr = htonl(dev.frame_ctr());

	send(&packet, sizeof(packet), peer);
}
