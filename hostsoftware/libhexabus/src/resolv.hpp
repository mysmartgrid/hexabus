#ifndef RESOLV_HPP_
#define RESOLV_HPP_ 1

#include <string>
#include <boost/asio.hpp>

namespace hexabus {

	boost::asio::ip::address_v6 resolve(boost::asio::io_service& io, const std::string& host, boost::system::error_code& err);

}

#endif
