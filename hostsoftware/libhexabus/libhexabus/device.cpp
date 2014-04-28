/**
 * This file is part of libhexabus.
 *
 * (c) Fraunhofer ITWM - Stephan Platz <platz@itwm.fhg.de>, 2014
 *
 * libhexabus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libhexabus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libhexabus. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "device.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

#include <libhexabus/filtering.hpp>

#include "../../../shared/endpoints.h"

using namespace hexabus;

bool isWrite(const Packet& packet, const boost::asio::ip::udp::endpoint& from)
{
	return packet.type() == HXB_PTYPE_WRITE;
}

bool dummy_write_handler(boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH>)
{
	return true;
}

Device::Device(boost::asio::io_service& io, const std::string& interface, const std::string& address, int interval)
	: _listener(io)
	, _socket(io)
	, _timer(io)
	, _interval(interval)
	, _sm_state(0)
{
	_listener.listen(interface);
	try {
		_socket.bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6::from_string(address), 61616));
		_socket.mcast_from(interface);
		_socket.onPacketReceived(boost::bind(&Device::_handle_query, this, _1, _2), filtering::isQuery() && (filtering::eid() % 32 > 0));
		_socket.onPacketReceived(boost::bind(&Device::_handle_write, this, _1, _2), isWrite && (filtering::eid() % 32 > 0));

		_socket.onPacketReceived(boost::bind(&Device::_handle_epquery, this, _1, _2), filtering::isEndpointQuery() && (filtering::eid() % 32 > 0));
		_listener.onPacketReceived(boost::bind(&Device::_handle_epquery, this, _1, _2), filtering::isEndpointQuery() && (filtering::eid() % 32 > 0));

		_socket.onPacketReceived(boost::bind(&Device::_handle_descquery, this, _1, _2), filtering::isQuery() && (filtering::eid() % 32 == 0));
		_socket.onPacketReceived(boost::bind(&Device::_handle_descepquery, this, _1, _2), filtering::isEndpointQuery() && (filtering::eid() % 32 == 0));
		_listener.onPacketReceived(boost::bind(&Device::_handle_descquery, this, _1, _2), filtering::isQuery() && (filtering::eid() % 32 == 0));
		_listener.onPacketReceived(boost::bind(&Device::_handle_descepquery, this, _1, _2), filtering::isEndpointQuery() && (filtering::eid() % 32 == 0));

		_socket.onPacketReceived(boost::bind(&Device::_handle_smupload, this, _1, _2), isWrite && filtering::eid() == EP_SM_UP_RECEIVER);

		_socket.onAsyncError(boost::bind(&Device::_handle_errors, this, _1));
	} catch ( const NetworkException& error ) {
		std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
		exit(1);
	}

	_handle_broadcasts(boost::system::error_code());

	TypedEndpointDescriptor<uint8_t>::Ptr smcontrolEP(new TypedEndpointDescriptor<uint8_t>(EP_SM_CONTROL, "Statemachine control"));
	smcontrolEP->onRead(boost::bind(&Device::_handle_smcontrolquery, this));
	smcontrolEP->onWrite(boost::bind(&Device::_handle_smcontrolwrite, this, _1));
	addEndpoint(smcontrolEP);
	//dummy endpoint to let _handle_descquery knows we can handle statemachine upload
	TypedEndpointDescriptor<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >::Ptr smupreceiverEP(new TypedEndpointDescriptor<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >(EP_SM_UP_RECEIVER, "Statemachine upload receiver"));
	smupreceiverEP->onWrite(&dummy_write_handler);
	addEndpoint(smupreceiverEP);
}

boost::signals2::connection Device::onReadName(const read_name_fn_t& callback)
{
	boost::signals2::connection result = _read.connect(callback);

	return result;
}

boost::signals2::connection Device::onWriteName(const write_name_fn_t& callback)
{
	boost::signals2::connection result = _write.connect(callback);

	return result;
}

void Device::addEndpoint(const EndpointDescriptor::Ptr ep)
{
	_endpoints.insert(std::pair<uint32_t, const EndpointDescriptor::Ptr>(ep->eid(), ep));
}

void Device::_handle_query(const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const QueryPacket* query = dynamic_cast<const QueryPacket*>(&p);
	if ( !query ) {
		//TODO: What to do?
		return;
	}
	std::map<uint32_t, const EndpointDescriptor::Ptr>::iterator it = _endpoints.find(query->eid());
	try {
		if ( it != _endpoints.end() ) {
			_socket.send(*(it->second->handle_query()), from);
		} else {
			std::cout << "unknown EID" << std::endl;
			_socket.send(ErrorPacket(HXB_ERR_UNKNOWNEID), from);
		}
	} catch ( const NetworkException& error ) {
		std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
	}
}

void Device::_handle_write(const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const EIDPacket* write = dynamic_cast<const EIDPacket*>(&p);
	if ( !write ) {
		//TODO: What to do?
		return;
	}
	std::map<uint32_t, const EndpointDescriptor::Ptr>::iterator it = _endpoints.find(write->eid());
	try {
		if ( it != _endpoints.end() ) {
			uint8_t res = it->second->handle_write(p);
			if ( res != HXB_ERR_SUCCESS && res < HXB_ERR_INTERNAL ) {
				std::cerr << "Error while writing " << write->eid() << std::endl;
				_socket.send(ErrorPacket(res), from);
			}
		} else {
			std::cout << "unknown EID" << std::endl;
			_socket.send(ErrorPacket(HXB_ERR_UNKNOWNEID), from);
		}
	} catch ( const NetworkException& error ) {
		std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
	}
}

void Device::_handle_epquery(const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const EIDPacket* query = dynamic_cast<const EIDPacket*>(&p);
	if ( !query ) {
		//TODO: What to do?
		return;
	}
	std::map<uint32_t, const EndpointDescriptor::Ptr>::iterator it = _endpoints.find(query->eid());
	try {
		if ( it != _endpoints.end() ) {
			//TODO: determine datatype
			_socket.send(EndpointInfoPacket(query->eid(), HXB_DTYPE_UINT32, it->second->name()), from);
		} else {
			std::cout << "unknown EID" << std::endl;
			_socket.send(ErrorPacket(HXB_ERR_UNKNOWNEID), from);
		}
	} catch ( const NetworkException& error ) {
		std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
	}
}

void Device::_handle_descquery(const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	std::cout << "blubb" << std::endl;
	const EIDPacket* query = dynamic_cast<const EIDPacket*>(&p);
	if ( !query ) {
		//TODO: What to do?
		return;
	}

	uint32_t group = 1;
	std::map<uint32_t, const EndpointDescriptor::Ptr>::iterator it = _endpoints.begin();
	while ( it->first < query->eid() && it != _endpoints.end() ) { it++; }
	while ( it->first < ( query->eid() + 32 ) && it != _endpoints.end() )
	{
		group |= (1 << (it->first - query->eid()) );
		it++;
	}

	try {
		_socket.send(InfoPacket<uint32_t>(query->eid(), group), from);
	} catch ( const NetworkException& error ) {
		std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
	}
}

void Device::_handle_descepquery(const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const EIDPacket* query = dynamic_cast<const EIDPacket*>(&p);
	if ( !query ) {
		//TODO: What to do?
		return;
	}

	std::string device_name = "";
	if ( _read.num_slots() > 0 )
		device_name = *_read();

	try {
		_socket.send(EndpointInfoPacket(query->eid(), HXB_DTYPE_UINT32, device_name), from);
	} catch ( const NetworkException& error ) {
		std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
	}
}

uint8_t Device::_handle_smcontrolquery()
{
	return _sm_state;
}

bool Device::_handle_smcontrolwrite(uint8_t value)
{
	_sm_state = value;
	return true;
}

void Device::_handle_smupload(const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const WritePacket<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >* write = dynamic_cast<const WritePacket<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >*>(&p);
	if ( !write ) {
		try {
			_socket.send(InfoPacket<bool>(EP_SM_UP_ACKNAK, false), from);
		} catch ( const NetworkException& error ) {
			std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
		}
		return;
	}
	const boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> data = write->value();
	if ( data[0] == 0 )
	{
		std::string name = "";
		int i = 0;
		while ( data[++i] != '\0' && i < HXB_65BYTES_PACKET_BUFFER_LENGTH )
		{
			name.push_back(data[i]);
		}
		if ( !name.empty() )
		{
			_write(name);
		}
	}
	try {
		_socket.send(InfoPacket<bool>(EP_SM_UP_ACKNAK, true), from);
	} catch ( const NetworkException& error ) {
		std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
	}
}

void Device::_handle_broadcasts(const boost::system::error_code& error)
{
	if ( !error )
	{
		for ( std::map<uint32_t, const EndpointDescriptor::Ptr>::iterator it = _endpoints.begin(), end = _endpoints.end(); it != end; ++it )
		{
			Packet::Ptr p = it->second->handle_query();
			if ( p && p->type() != HXB_PTYPE_ERROR )
			{
				try {
					_socket.send(*p);
				} catch ( const NetworkException& error ) {
					std::cerr << "An error occured during " << error.reason() << ": " << error.code().message() << std::endl;
				}
			}
		}

		_timer.expires_from_now(boost::posix_time::seconds(_interval+(rand()%_interval)));
		_timer.async_wait(boost::bind(&Device::_handle_broadcasts, this, _1));
	}
}

void Device::_handle_errors(const GenericException& error)
{
	const NetworkException* nerror;
	if ((nerror = dynamic_cast<const NetworkException*>(&error))) {
		std::cerr << "Error while receiving hexabus packets: " << nerror->code().message() << std::endl;
	} else {
		std::cerr << "Error while receiving hexabus packets: " << error.what() << std::endl;
	}
}

