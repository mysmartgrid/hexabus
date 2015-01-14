#include "resolv.hpp"

namespace hexabus {

boost::asio::ip::address_v6 resolve(boost::asio::io_service& io, const std::string& host, boost::system::error_code& err)
{
	boost::asio::ip::udp::resolver resolver(io);

	boost::asio::ip::udp::resolver::query query(host, "");

	boost::asio::ip::udp::resolver::iterator it, end;

	it = resolver.resolve(query, err);
	if (err)
		return boost::asio::ip::address_v6::any();

	if (it == end || !it->endpoint().address().is_v6()) {
		err = boost::system::error_code(boost::system::errc::invalid_argument, boost::system::generic_category());
		return boost::asio::ip::address_v6::any();
	}

	return it->endpoint().address().to_v6();
}

}
