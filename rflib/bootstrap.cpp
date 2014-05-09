#include "bootstrap.hpp"

#include "nlmessages.hpp"
#include "nlparsers.hpp"

#include "boost/tuple/tuple.hpp"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <time.h>

#include <stdexcept>
#include <limits>

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

bool BootstrapSocket::receive_wait(int timeout)
{
	pollfd pfd;

	pfd.fd = _fd;
	pfd.events = POLLIN;

	int rc = poll(&pfd, 1, timeout);

	if (rc < 0)
		throw std::runtime_error(strerror(errno));

	return rc > 0;
}

void BootstrapSocket::bind(const ieee802154_addr& addr)
{
	sockaddr_ieee802154 saddr;
	sockaddr* ap = reinterpret_cast<sockaddr*>(&saddr);

	saddr.family = AF_IEEE802154;
	saddr.addr = addr;

	if (::bind(_fd, ap, sizeof(saddr)) < 0)
		throw std::runtime_error(strerror(errno));
}

std::pair<std::vector<uint8_t>, sockaddr_ieee802154> BootstrapSocket::receive()
{
	std::vector<uint8_t> packet(128, 0);
	sockaddr_ieee802154 peer;
	socklen_t peerlen = sizeof(peer);
	sockaddr* addr = reinterpret_cast<sockaddr*>(&peer);

	int len = 0;
	do {
		len = recvfrom(_fd, &packet[0], packet.size(), 0, addr, &peerlen);
	} while (len < 0 && errno == EINTR);
	if (len < 0)
		throw std::runtime_error(strerror(errno));

	packet.resize(len);
	return std::make_pair(packet, peer);
}

void BootstrapSocket::send(const void* msg, size_t len, const sockaddr_ieee802154& peer)
{
	socklen_t peerlen = sizeof(peer);
	const sockaddr* addr = reinterpret_cast<const sockaddr*>(&peer);

	ssize_t sent = 0;
	do {
		sent = sendto(_fd, msg, len, 0, addr, peerlen);
	} while (sent < 0 && errno == EINTR);
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

void PairingHandler::run_once(int timeout_secs)
{
	struct {
		uint8_t msg;
		uint16_t pan_id;
		uint8_t key[16];
	} __attribute__((packed)) packet;

	std::vector<uint8_t> recv_data;
	sockaddr_ieee802154 peer;
	int timeout;

	if (timeout_secs > std::numeric_limits<int>::max() / 1000)
		timeout = std::numeric_limits<int>::max() / 1000;
	else
		timeout = timeout_secs * 1000;

	timespec ts, ts2;

	if (clock_gettime(CLOCK_MONOTONIC, &ts))
		throw std::runtime_error(strerror(errno));

	for (;;) {
		if (clock_gettime(CLOCK_MONOTONIC, &ts2))
			throw std::runtime_error(strerror(errno));

		timeout -= (ts2.tv_sec - ts.tv_sec) * 1000;
		timeout -= ts2.tv_nsec / 1000000 - ts.tv_nsec / 1000000;

		ts = ts2;
		if (!receive_wait(timeout))
			throw std::runtime_error("timeout");

		boost::tie(recv_data, peer) = receive();
		if (recv_data.size() && recv_data[0] == HXB_B_PAIR_REQUEST)
			break;
	}

	if (recv_data.size() != 1)
		throw std::runtime_error("invalid PAIR_REQUEST");

	packet.msg = HXB_B_PAIR_RESPONSE;
	packet.pan_id = htons(_pan_id);

	Key key = control().get_out_key(iface());
	memcpy(packet.key, key.key(), 16);

	uint64_t addr;
	memcpy(&addr, peer.addr.hwaddr, 8);

	try {
		Device dev(_pan_id, 0xfffe, addr, 0, false, IEEE802154_LLSEC_DEVKEY_IGNORE);

		control().add_device(iface(), dev);
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
	if (recv_data.size() != 1 || recv_data[0] != HXB_B_RESYNC_REQUEST)
		return;

	packet.msg = HXB_B_RESYNC_RESPONSE;

	uint64_t addr;

	memcpy(&addr, peer.addr.hwaddr, 8);
	Device dev = control().list_devices(iface(), addr).at(0);

	packet.ctr = htonl(dev.frame_ctr());

	send(&packet, sizeof(packet), peer);
}

void ResyncHandler::force(int attempts, int interval)
{
	attempts = std::max(0, attempts);

	sockaddr_ieee802154 all;
	all.family = AF_IEEE802154;
	all.addr.addr_type = IEEE802154_ADDR_SHORT;
	all.addr.pan_id = 0xffff;
	all.addr.short_addr = 0xffff;

	while (attempts-- > 0) {
		uint8_t msg = HXB_B_RESYNC_EXTERN;
		send(&msg, sizeof(msg), all);

		int rest = interval;
		while (rest > 0)
			rest = sleep(rest);
	}
}
