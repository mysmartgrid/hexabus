#include "device_interrogator.hpp"

#include <algorithm>

#include <boost/scope_exit.hpp>

#include "filtering.hpp"
#include "../../../shared/endpoints.h"

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
	DeviceInterrogator* _this = this;

	BOOST_SCOPE_EXIT((&running_queries) (&order_destroyed) (_this)) {
		if (order_destroyed) {
			std::make_heap(running_queries.begin(), running_queries.end());
		}
		_this->reschedule_timer();
	} BOOST_SCOPE_EXIT_END

	while (it != running_queries.end()) {
		base_query& q = *it;

		if (q.filter(packet, from)) {
			BOOST_SCOPE_EXIT((&running_queries) (&order_destroyed) (&it)) {
				running_queries.erase(it);
				order_destroyed = true;
			} BOOST_SCOPE_EXIT_END

			q.response(packet);
		} else {
			it++;
		}
	}
}

void DeviceInterrogator::timeout(const boost::system::error_code& err)
{
	if (!err && running_queries.size()) {
		boost::posix_time::ptime now = boost::asio::deadline_timer::traits_type::now();

		std::pop_heap(running_queries.begin(), running_queries.end());
		base_query& back = running_queries.back();

		DeviceInterrogator* _this = this;
		BOOST_SCOPE_EXIT((_this)) {
			_this->reschedule_timer();
		} BOOST_SCOPE_EXIT_END

		if (back.retries++ == back.max_retries) {
			BOOST_SCOPE_EXIT((&running_queries)) {
				running_queries.pop_back();
			} BOOST_SCOPE_EXIT_END

			back.failure(NetworkException("Device not responding",
						boost::system::error_code(boost::system::errc::timed_out, boost::system::generic_category())));
		} else {
			try {
				socket.send(*back.packet, back.device);
			} catch (const GenericException& e) {
				back.failure(e);
				running_queries.pop_back();
				return;
			} catch (...) {
				running_queries.pop_back();
				throw;
			}

			back.deadline += boost::posix_time::seconds(1);
			std::push_heap(running_queries.begin(), running_queries.end());
		}
	}
}

void DeviceInterrogator::reschedule_timer()
{
	if (running_queries.size()) {
		timer.expires_at(running_queries[0].deadline);
		timer.async_wait(boost::bind(&DeviceInterrogator::timeout, this, _1));
	}
}

void DeviceInterrogator::queue_query(
		const boost::asio::ip::address_v6& device,
		const Packet::Ptr& packet,
		const Socket::filter_t& filter,
		const std::function<void (const Packet&)>& response_cb,
		const std::function<void (const GenericException&)>& failure_cb,
		int max_tries)
{
	base_query query;
	query.retries = 0;
	query.max_retries = max_tries - 1;
	query.device = device;
	query.deadline = boost::asio::deadline_timer::traits_type::now() + boost::posix_time::seconds(1);
	query.filter = filter;
	query.packet = packet;
	query.response = response_cb;
	query.failure = failure_cb;

	try {
		socket.send(*query.packet, query.device);
	} catch (const GenericException& e) {
		failure_cb(e);
		return;
	}

	running_queries.push_back(query);
	std::push_heap(running_queries.begin(), running_queries.end());

	if (running_queries.size() == 1) {
		reschedule_timer();
	}
}
