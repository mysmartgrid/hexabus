#include "device_interrogator.hpp"

#include <algorithm>

#include <libhexabus/filtering.hpp>
#include "../../../../shared/endpoints.h"

using namespace hexanode;
namespace hf = hexabus::filtering;


DeviceInterrogator::DeviceInterrogator(hexabus::Socket& socket)
	: socket(socket),
		ep_info_timer(socket.ioService()),
		ep_info_listener(
				socket.onPacketReceived(
					boost::bind(&DeviceInterrogator::ep_info, this, _1, _2),
					hf::isEndpointInfo()))
{
}

void DeviceInterrogator::get_device_name(
		const boost::asio::ip::address_v6& device,
		const boost::function<void (const std::string&)>& response_cb,
		const boost::function<void (const GenericException&)>& failure_cb,
		int max_retries)
{
	if (running_name_queries.count(device)) {
		name_query& query = running_name_queries[device];
		std::cout << "Name query for " << device << " in progress, adjusting retry count" << std::endl;
		query.max_retries = std::max(query.max_retries, query.retries + max_retries);
	} else {
		std::cout << "Running name query for " << device << std::endl;
		name_query query = {
			0,
			max_retries,
			device,
			response_cb,
			failure_cb
		};

		running_name_queries_t::iterator it = running_name_queries.insert(std::make_pair(device, query)).first;

		try_ep_query(it->second);
	}
}

void DeviceInterrogator::try_ep_query(name_query& query)
{
	query.deadline = boost::posix_time::microsec_clock::local_time() + boost::posix_time::seconds(1);

	std::cout << "Sending EPQuery(" << EP_DEVICE_DESCRIPTOR << ") to " << query.device << ", try #" << query.retries << std::endl;
	socket.send(hexabus::EndpointQueryPacket(EP_DEVICE_DESCRIPTOR), query.device);
	restart_ep_info_timer();
}

template<typename Query>
static bool deadline_comp(const std::pair<boost::asio::ip::address_v6, Query>& lhs,
		const std::pair<boost::asio::ip::address_v6, Query>& rhs)
{
	return lhs.second.deadline < rhs.second.deadline;
}

DeviceInterrogator::running_name_queries_t::iterator DeviceInterrogator::next_ep_query()
{
	return std::min_element(running_name_queries.begin(), running_name_queries.end(), deadline_comp<name_query>);
}

void DeviceInterrogator::restart_ep_info_timer()
{
	if (running_name_queries.size()) {
		name_query& next = next_ep_query()->second;

		ep_info_timer.expires_at(next.deadline);
		ep_info_timer.async_wait(boost::bind(&DeviceInterrogator::ep_info_timeout, this, _1));
	}
}

void DeviceInterrogator::ep_info_timeout(const boost::system::error_code& error)
{
	if (!error) {
		running_name_queries_t::iterator next = next_ep_query();

		name_query& query = next->second;
		query.retries++;
		if (query.retries == query.max_retries) {
			std::cout << "No response to EPQuery(" << EP_DEVICE_DESCRIPTOR << ") from " << query.device << std::endl;
			running_name_queries.erase(next);
			query.failure_cb(CommunicationException("Device not responding"));
		} else {
			try_ep_query(query);
		}
	}
}

void DeviceInterrogator::ep_info(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& endpoint)
{
	running_name_queries_t::iterator device = running_name_queries.find(endpoint.address().to_v6());

	if (device != running_name_queries.end()) {
		device->second.response_cb(static_cast<const hexabus::EndpointInfoPacket&>(packet).value());
		running_name_queries.erase(device);
	}
}
