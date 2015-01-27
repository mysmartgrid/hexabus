#ifndef LIBHEXABUS_DEVICE__INTERROGATOR_HPP
#define LIBHEXABUS_DEVICE__INTERROGATOR_HPP 1

#include <functional>
#include <string>
#include <vector>

#include "error.hpp"

#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <libhexabus/socket.hpp>

namespace hexabus {

class DeviceInterrogator {
	private:
		struct base_query {
			int retries;
			int max_retries;
			boost::asio::ip::address_v6 device;

			boost::posix_time::ptime deadline;

			Packet::Ptr packet;
			Socket::filter_t filter;

			std::function<void (const Packet&)> response;
			std::function<void (const GenericException&)> failure;

			bool operator<(const base_query& other) { return other.deadline < deadline; }
		};

		typedef std::vector<base_query> running_queries_t;

	private:
		hexabus::Socket& socket;
		boost::asio::deadline_timer timer;
		boost::signals2::scoped_connection on_reply;

		running_queries_t running_queries;

		void packet_received(const Packet& packet, const boost::asio::ip::udp::endpoint& from);
		void timeout(const boost::system::error_code& err);

		void reschedule_timer();

		void queue_query(
				const boost::asio::ip::address_v6& device,
				const Packet::Ptr& packet,
				const Socket::filter_t& filter,
				const std::function<void (const Packet&)>& response_cb,
				const std::function<void (const GenericException&)>& failure_cb,
				int max_tries);

	public:
		DeviceInterrogator(hexabus::Socket& socket);

		template<typename Packet, typename Filter>
		void send_request(
				const boost::asio::ip::address_v6& device,
				const Packet packet,
				const Filter filter,
				const std::function<void (const hexabus::Packet&)>& response_cb,
				const std::function<void (const GenericException&)>& failure_cb,
				int max_tries = 5)
		{
			queue_query(
					device,
					hexabus::Packet::Ptr(new Packet(packet)),
					hexabus::filtering::sourceIP() == device && filter,
					response_cb,
					failure_cb,
					max_tries);
		}
};

}

#endif
