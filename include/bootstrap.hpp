#ifndef BOOTSTRAP_HPP_
#define BOOTSTRAP_HPP_

#include "ieee802154.h"
#include "netlink.hpp"
#include "controller.hpp"
#include "fd.hpp"

#include <unistd.h>
#include <fcntl.h>

#include <vector>
#include <string>

class BootstrapSocket {
	private:
		FD _fd;
		std::string _iface;
		Controller _control;

	protected:
		enum {
			HXB_B_PAIR_REQUEST = 0,
			HXB_B_PAIR_RESPONSE = 1,
		} bootstrap_messages;

	protected:
		BootstrapSocket(const std::string& iface, bool nosec = false);
		virtual ~BootstrapSocket() {}

		const std::string& iface() const { return _iface; }
		Controller& control() { return _control; }

		bool receive_wait(int timeout = -1);
		std::pair<std::vector<uint8_t>, sockaddr_ieee802154> receive();
		void send(const void* msg, size_t len, const sockaddr_ieee802154& peer);

		int fd() const { return _fd; }

	public:
		void bind(const ieee802154_addr& addr);
};

class PairingHandler : public BootstrapSocket {
	private:
		uint16_t _pan_id;

	public:
		PairingHandler(const std::string& iface, uint16_t pan_id);

		void run_once(int timeout_secs);
};

#endif
