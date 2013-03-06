#include "hexabus_server.hpp"

#include <syslog.h>

#include <libhexabus/filtering.hpp>

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

using namespace hexadaemon;

namespace hf = hexabus::filtering;
namespace bf = boost::filesystem;

HexabusServer::HexabusServer(boost::asio::io_service& io)
	: _socket(io)
	, _timer(io)
{
	_socket.listen(boost::asio::ip::address_v6::from_string("::1"));
	_socket.onPacketReceived(boost::bind(&HexabusServer::epqueryhandler, this, _1, _2), hf::isEndpointQuery());
	_socket.onPacketReceived(boost::bind(&HexabusServer::eid0handler, this, _1, _2), hf::isQuery() && hf::eid() == 0);
	_socket.onPacketReceived(boost::bind(&HexabusServer::eid2handler, this, _1, _2), hf::isQuery() && hf::eid() == 2);

	/*_timer.expires_from_now(boost::posix_time::seconds(60+(rand()%60)));
	_timer.async_wait(boost::bind(&HexabusServer::broadcast_handler, this, _1));*/
	broadcast_handler(boost::system::error_code());
}

void HexabusServer::epqueryhandler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const hexabus::EndpointQueryPacket* packet = dynamic_cast<const hexabus::EndpointQueryPacket*>(&p);
	try {
		switch (packet->eid()) {
			case 0:
				_socket.send(hexabus::EndpointInfoPacket(0, 6, "HexaDaemon", 0), from);
				break;
			case 2:
				_socket.send(hexabus::EndpointInfoPacket(2, 3, "Power meter (W)", 0), from);
				break;
		}
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::eid0handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	std::cout << "EID 0 packet received" << std::endl;
	try {
	_socket.send(hexabus::InfoPacket<uint32_t>(0, 2), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::eid2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	std::cout << "EID 2 packet received" << std::endl;
	try {
		int value = getFluksoValue();
		if ( value >= 0 )
			_socket.send(hexabus::InfoPacket<uint32_t>(2, value));
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::broadcast_handler(const boost::system::error_code& error)
{
	std::cout << "Broadcast_handler!" << std::endl;
	if ( !error )
	{
		int value = getFluksoValue();
		if ( value >= 0 )
			_socket.send(hexabus::InfoPacket<uint32_t>(2, value));

		_timer.expires_from_now(boost::posix_time::seconds(60+(rand()%60)));
		_timer.async_wait(boost::bind(&HexabusServer::broadcast_handler, this, _1));
	} else {
		std::cout << "Error occured in deadline_timer: " << error.message() << "(" << error.value() << ")." << std::endl;
	}
}

int HexabusServer::getFluksoValue()
{
	updateFluksoValues();
	int result = 0;

	for ( std::map<std::string, uint16_t>::iterator it = _flukso_values.begin(); it != _flukso_values.end(); it++ )
		result += it->second;

	return result;
}

void HexabusServer::updateFluksoValues()
{
	bf::path p("/var/run/fluksod/sensor/");

	if ( exists(p) && is_directory(p) )
	{
		std::ifstream file;
		for ( bf::directory_iterator sensors(p); sensors != bf::directory_iterator(); sensors++ )
		{
			std::string filename = (*sensors).path().filename().string();

			boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> data;
			unsigned short hash;
			for ( unsigned int pos = 0; pos < 16; pos++ )
			{
				std::stringstream ss(filename.substr(2*pos, 2));
				ss >> std::hex >> hash;
				data[pos] = hash;
			}

			file.open((*sensors).path().string().c_str());
			std::string flukso_data;
			file >> flukso_data;
			file.close();
			std::cout << "flukso_data: " << flukso_data << std::endl;
			boost::regex r("^\\[(?:\\[[[:digit:]]*,[[:digit:]]*\\],)*\\[[[:digit:]]*,([[:digit:]]*)\\],(?:\\[[[:digit:]]*,\"nan\"\\],?)+\\]$");
			boost::match_results<std::string::const_iterator> what;

			if ( !boost::regex_search(flukso_data, what, r))
				break;

			uint16_t value = boost::lexical_cast<uint16_t>(std::string(what[1].first, what[1].second));

			_flukso_values[filename] = value;

			union {
				uint16_t u16;
				char raw[sizeof(u16)];
			} c = { htons(value) };
			for ( unsigned int pos = 0; pos < sizeof(uint16_t); pos++ )
			{
				data[16+pos] = c.raw[pos];
			}

			for ( unsigned int pos = 16 + sizeof(uint16_t); pos < HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH; pos++ )
				data[pos] = 0;

			_socket.send(hexabus::InfoPacket< boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >(21, data));
		}
	}
}
