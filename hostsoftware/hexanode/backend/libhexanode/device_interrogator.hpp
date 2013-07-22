#ifndef LIBHEXANODE_DEVICE__INTERROGATOR_HPP
#define LIBHEXANODE_DEVICE__INTERROGATOR_HPP 1

#include <string>
#include <map>

#include <libhexanode/error.hpp>

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <libhexabus/socket.hpp>

namespace hexanode {

class DeviceInterrogator {
	private:
		struct name_query {
			int retries;
			int max_retries;
			boost::asio::ip::address_v6 device;
			boost::function<void (const std::string&)> response_cb;
			boost::function<void (const GenericException&)> failure_cb;

			boost::posix_time::ptime deadline;
		};

		typedef std::map<boost::asio::ip::address_v6, name_query> running_name_queries_t;

		void restart_ep_info_timer();

		void try_ep_query(name_query& query);
		void ep_info_timeout(const boost::system::error_code& error);
		void ep_info(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint&);
		running_name_queries_t::iterator next_ep_query();

	private:
		hexabus::Socket& socket;
		boost::asio::deadline_timer ep_info_timer;
		boost::signals2::scoped_connection ep_info_listener;

		running_name_queries_t running_name_queries;

	public:
		DeviceInterrogator(hexabus::Socket& socket);

		void get_device_name(
				const boost::asio::ip::address_v6& device,
				const boost::function<void (const std::string&)>& response_cb,
				const boost::function<void (const GenericException&)>& failure_cb,
				int max_retries = 5);
};

}

#endif
