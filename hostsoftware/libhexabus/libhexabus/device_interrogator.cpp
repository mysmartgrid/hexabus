#include "device_interrogator.hpp"

#include <algorithm>

#include "filtering.hpp"
#include "../../../../shared/endpoints.h"

using namespace hexabus;

DeviceInterrogator::DeviceInterrogator(hexabus::Socket& socket)
	: socket(socket),
		timer(socket.ioService()),
		on_reply(
				socket.onPacketReceived(
					boost::bind(&DeviceInterrogator::packet_received, this, _1, _2)))
{
}

void DeviceInterrogator::packet_received(const Packet& packet, const boost::asio::ip::udp::endpoint& from)
{
	running_queries_t::iterator it = running_queries.begin();
	bool order_destroyed = false;

	while (it != running_queries.end()) {
		base_query& q = *it;

		if (q.filter(packet, from)) {
			q.response(packet);
			running_queries.erase(it);
			order_destroyed = true;
		} else {
			it++;
		}
	}

	if (order_destroyed) {
		std::make_heap(running_queries.begin(), running_queries.end());
	}
	if (running_queries.size()) {
		reschedule_timer();
	}
}

void DeviceInterrogator::timeout(const boost::system::error_code& err)
{
	if (!err && running_queries.size()) {
		std::pop_heap(running_queries.begin(), running_queries.end());
		base_query& back = running_queries.back();

		if (back.retries++ == back.max_retries) {
			back.failure(NetworkException("Device not responding",
						boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category())));
			running_queries.pop_back();
		} else {
			back.deadline += boost::posix_time::seconds(1);
			std::push_heap(running_queries.begin(), running_queries.end());
			socket.send(*back.packet, back.device);
			reschedule_timer();
		}
	}
}

void DeviceInterrogator::reschedule_timer()
{
	timer.expires_at(running_queries[0].deadline);
	timer.async_wait(boost::bind(&DeviceInterrogator::timeout, this, _1));
}

void DeviceInterrogator::queue_query(
		const boost::asio::ip::address_v6& device,
		const Packet::Ptr& packet,
		const Socket::filter_t& filter,
		const boost::function<void (const Packet&)>& response_cb,
		const boost::function<void (const GenericException&)>& failure_cb,
		int max_tries)
{
	base_query query = { 0 };
	query.max_retries = max_tries - 1;
	query.device = device;
	query.filter = filter;
	query.packet = packet;
	query.response = response_cb;
	query.failure = failure_cb;
	query.deadline = boost::asio::deadline_timer::traits_type::now() + boost::posix_time::seconds(1);

	running_queries.push_back(query);
	std::push_heap(running_queries.begin(), running_queries.end());
	socket.send(*query.packet, query.device);

	if (running_queries.size() == 1) {
		reschedule_timer();
	}
}
