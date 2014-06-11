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

bool BootstrapSocket::receive_wait(const timespec* timeout)
{
	pollfd pfd;

	pfd.fd = _fd;
	pfd.events = POLLIN;

	int rc = ppoll(&pfd, 1, timeout, NULL);

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



PairingHandler::PairingHandler(const std::string& iface, Network& net)
	: BootstrapSocket(iface, true), _net(net)
{
}

void PairingHandler::run_once(int timeout_secs)
{
	struct {
		uint8_t tag;
		uint8_t type;
	} __attribute__((packed)) query;

	struct {
		uint8_t tag;
		uint8_t type;
		uint16_t pan_id;
		uint8_t key[16];
		uint64_t key_epoch;
	} __attribute__((packed)) response;

	std::vector<uint8_t> recv_data;
	sockaddr_ieee802154 peer;
	timespec started, timeout;

	timeout.tv_sec = timeout_secs;
	timeout.tv_nsec = 0;

	if (clock_gettime(CLOCK_MONOTONIC, &started))
		throw std::runtime_error(strerror(errno));

	for (;;) {
		if (!receive_wait(timeout_secs < 0 ? NULL : &timeout))
			throw std::runtime_error("timeout");

		boost::tie(recv_data, peer) = receive();
		if (recv_data.size() == sizeof(query)) {
			memcpy(&query, &recv_data[0], sizeof(query));
			if (query.tag == HXB_B_TAG && query.type == HXB_B_PAIR_REQUEST)
				break;
		}

		timespec now;
		if (clock_gettime(CLOCK_MONOTONIC, &now))
			throw std::runtime_error(strerror(errno));

		timeout.tv_nsec -= (now.tv_nsec - started.tv_nsec);
		if (timeout.tv_nsec < 0) {
			timeout.tv_nsec += 1000 * 1000 * 1000;
			timeout.tv_sec--;
		}
		timeout.tv_sec -= (now.tv_sec - started.tv_sec);

		if (timeout.tv_sec < 0)
			throw std::runtime_error("timeout");

		started = now;
	}

	response.tag = HXB_B_TAG;
	response.type = HXB_B_PAIR_RESPONSE;
	response.pan_id = htobe16(_net.pan().pan_id());
	memcpy(response.key, &_net.master_key()[0], 16);
	response.key_epoch = htobe64(_net.key_epoch());

	uint64_t addr;
	memcpy(&addr, peer.addr.hwaddr, 8);

	try {
		const Device& dev = _net.add_device(addr);

		control().add_device(iface(), dev);
	} catch (const nl::nl_error& e) {
		if (e.error() != NLE_EXIST)
			throw;
	}

	send(&response, sizeof(response), peer);
}
