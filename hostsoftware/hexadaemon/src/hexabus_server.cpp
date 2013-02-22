#include "hexabus_server.hpp"

#include <syslog.h>

#include <libhexabus/filtering.hpp>

using namespace hexadaemon;

namespace hf = hexabus::filtering;

HexabusServer::HexabusServer(boost::asio::io_service& io)
	: _socket(io)
{
	_socket.listen(boost::asio::ip::address_v6::from_string("::1"));
	_socket.onPacketReceived(boost::bind(&HexabusServer::epqueryhandler, this, _1, _2), hf::isEndpointQuery());
	_socket.onPacketReceived(boost::bind(&HexabusServer::eid0handler, this, _1, _2), hf::isQuery() && hf::eid() == 0);
	_socket.onPacketReceived(boost::bind(&HexabusServer::eid2handler, this, _1, _2), hf::isQuery() && hf::eid() == 2);
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
	//_socket.send(hexabus::ValuePacket<uint32_t>(0, 5), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}

void HexabusServer::eid2handler(const hexabus::Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	std::cout << "EID 2 packet received" << std::endl;
	try {
	//_socket.send(hexabus::ValuePacket<uint32_t>(2, 17), from);
	} catch ( const hexabus::NetworkException& e ) {
		std::cerr << "Could not send packet to " << from << ": " << e.code().message() << std::endl;
	}
}
