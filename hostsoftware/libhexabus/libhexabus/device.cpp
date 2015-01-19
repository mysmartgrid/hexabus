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
#include "../../../shared/hexabus_types.h"
#include "../../../shared/hexabus_definitions.h"

using namespace hexabus;

bool isWrite(const Packet& packet, const boost::asio::ip::udp::endpoint& from)
{
	return packet.type() == HXB_PTYPE_WRITE;
}

bool dummy_write_handler(const boost::array<uint8_t, 65>& value)
{
	return true;
}

Device::Device(boost::asio::io_service& io, const std::vector<std::string>& interfaces, const std::vector<std::string>& addresses, int interval)
	: _listener(io)
	, _sockets()
	, _timer(io)
	, _interval(interval)
	, _sm_state(0)
{
	for (std::vector<std::string>::const_iterator it = interfaces.begin(), end = interfaces.end(); it != end; ++it) {
		try {
			_listener.listen(*it);
		} catch ( const NetworkException& error ) {
			std::stringstream oss;
			oss << "An error occured when trying to listen on " << *it << ": " << error.reason() << ": " << error.code().message();
			throw(GenericException(oss.str()));
		}
	}
	for (std::vector<std::string>::const_iterator it = addresses.begin(), end = addresses.end(); it != end; ++it) {
		hexabus::Socket *socket = 0;
		try {
			socket = new hexabus::Socket(io);
			socket->bind(boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6::from_string(*it), 61616));
			socket->onPacketReceived(boost::bind(&Device::_handle_query, this, socket, _1, _2), filtering::isQuery() && (filtering::eid() % 32 > 0));
			socket->onPacketReceived(boost::bind(&Device::_handle_write, this, socket, _1, _2), isWrite && (filtering::eid() % 32 > 0));

			socket->onPacketReceived(boost::bind(&Device::_handle_epquery, this, socket, _1, _2), filtering::isEndpointQuery() && (filtering::eid() % 32 > 0));

			socket->onPacketReceived(boost::bind(&Device::_handle_descquery, this, socket, _1, _2), filtering::isQuery() && (filtering::eid() % 32 == 0));
			socket->onPacketReceived(boost::bind(&Device::_handle_descepquery, this, socket, _1, _2), filtering::isEndpointQuery() && (filtering::eid() % 32 == 0));

			socket->onPacketReceived(boost::bind(&Device::_handle_smupload, this, socket, _1, _2), isWrite && filtering::eid() == EP_SM_UP_RECEIVER);

			socket->onAsyncError(boost::bind(&Device::_handle_errors, this, _1));
			_sockets.push_back(socket);
		} catch ( const NetworkException& error ) {
			if ( socket )
				delete socket;

			std::stringstream oss;
			oss << "An error occured when opening socket for address " << *it << ": " << error.reason() << ": " << error.code().message();
			throw(GenericException(oss.str()));
		}
	}
	_listener.onPacketReceived(boost::bind(&Device::_handle_epquery, this, _1, _2), filtering::isEndpointQuery() && (filtering::eid() % 32 > 0));
	_listener.onPacketReceived(boost::bind(&Device::_handle_descquery, this, _1, _2), filtering::isQuery() && (filtering::eid() % 32 == 0));
	_listener.onPacketReceived(boost::bind(&Device::_handle_descepquery, this, _1, _2), filtering::isEndpointQuery() && (filtering::eid() % 32 == 0));

	_handle_broadcasts(boost::system::error_code());

	TypedEndpointFunctions<uint8_t>::Ptr smcontrolEP(new TypedEndpointFunctions<uint8_t>(EP_SM_CONTROL, "Statemachine control", false));
	smcontrolEP->onRead(boost::bind(&Device::_handle_smcontrolquery, this));
	smcontrolEP->onWrite(boost::bind(&Device::_handle_smcontrolwrite, this, _1));
	addEndpoint(smcontrolEP);
	//dummy endpoint to let _handle_descquery knows we can handle statemachine upload
	TypedEndpointFunctions<boost::array<uint8_t, 65> >::Ptr smupreceiverEP(new TypedEndpointFunctions<boost::array<uint8_t, 65> >(EP_SM_UP_RECEIVER, "Statemachine upload receiver", false));
	smupreceiverEP->onWrite(&dummy_write_handler);
	addEndpoint(smupreceiverEP);
}

Device::~Device()
{
	for (std::vector<hexabus::Socket*>::iterator it = _sockets.begin(), end = _sockets.end(); it != end; ++it) {
		delete *it;
	}
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

boost::signals2::connection Device::onAsyncError(const async_error_fn_t& callback)
{
	boost::signals2::connection result = _asyncError.connect(callback);

	return result;
}

void Device::addEndpoint(const EndpointFunctions::Ptr ep)
{
	_endpoints.insert(std::pair<uint32_t, const EndpointFunctions::Ptr>(ep->eid(), ep));
}

void Device::_handle_query(hexabus::Socket* socket, const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const QueryPacket* query = dynamic_cast<const QueryPacket*>(&p);
	if ( !query ) {
		_asyncError(GenericException("Trying to handle a query but didn't get a QueryPacket"));
		return;
	}
	std::map<uint32_t, const EndpointFunctions::Ptr>::iterator it = _endpoints.find(query->eid());
	try {
		if ( it != _endpoints.end() ) {
			socket->send(*(it->second->handle_query()), from);
		} else {
			socket->send(ErrorPacket(HXB_ERR_UNKNOWNEID), from);
		}
	} catch ( const NetworkException& error ) {
		std::stringstream oss;
		oss << "An error occured when replying a query: " << error.reason() << ": " << error.code().message();
		_asyncError(GenericException(oss.str()));
	} catch ( const GenericException& error ) {
		std::stringstream oss;
		oss << "An error occured when handling a query: " << error.reason();
		_asyncError(GenericException(oss.str()));
	}
}

void Device::_handle_write(hexabus::Socket* socket, const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const EIDPacket* write = dynamic_cast<const EIDPacket*>(&p);
	if ( !write ) {
		_asyncError(GenericException("Trying to handle a write but didn't get an EIDPacket"));
		return;
	}
	std::map<uint32_t, const EndpointFunctions::Ptr>::iterator it = _endpoints.find(write->eid());
	try {
		if ( it != _endpoints.end() ) {
			uint8_t res = it->second->handle_write(p);
			if ( res != HXB_ERR_SUCCESS && res < HXB_ERR_INTERNAL ) {
				std::stringstream oss;
				oss << "An error occured when handling a write packet for eid " << write->eid();
				_asyncError(GenericException(oss.str()));
				socket->send(ErrorPacket(res), from);
			}
		} else {
			socket->send(ErrorPacket(HXB_ERR_UNKNOWNEID), from);
		}
	} catch ( const NetworkException& error ) {
		std::stringstream oss;
		oss << "An error occured when replying to a write packet: " << error.reason() << ": " << error.code().message();
		_asyncError(GenericException(oss.str()));
	} catch ( const GenericException& error ) {
		std::stringstream oss;
		oss << "An error occured when handling a write: " << error.reason();
		_asyncError(GenericException(oss.str()));
	}
}

void Device::_handle_epquery(const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	for ( std::vector<hexabus::Socket*>::iterator it = _sockets.begin(), end = _sockets.end(); it != end; ++it )
	{
		_handle_epquery(*it, p, from);
	}
}

void Device::_handle_epquery(hexabus::Socket* socket, const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const EIDPacket* query = dynamic_cast<const EIDPacket*>(&p);
	if ( !query ) {
		_asyncError(GenericException("Trying to handle an EP query but didn't get an EIDPacket"));
		return;
	}
	std::map<uint32_t, const EndpointFunctions::Ptr>::iterator it = _endpoints.find(query->eid());
	try {
		if ( it != _endpoints.end() ) {
			socket->send(EndpointInfoPacket(query->eid(), it->second->datatype(), it->second->name()), from);
		} else {
			socket->send(ErrorPacket(HXB_ERR_UNKNOWNEID), from);
		}
	} catch ( const NetworkException& error ) {
		std::stringstream oss;
		oss << "An error occured when replying to an EP query packet: " << error.reason() << ": " << error.code().message();
		_asyncError(GenericException(oss.str()));
	}
}

void Device::_handle_descquery(const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	for ( std::vector<hexabus::Socket*>::iterator it = _sockets.begin(), end = _sockets.end(); it != end; ++it )
	{
		_handle_descquery(*it, p, from);
	}
}

void Device::_handle_descquery(hexabus::Socket* socket, const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const EIDPacket* query = dynamic_cast<const EIDPacket*>(&p);
	if ( !query ) {
		_asyncError(GenericException("Trying to handle an EP group query but didn't get an EIDPacket"));
		return;
	}

	uint32_t group = 1;
	std::map<uint32_t, const EndpointFunctions::Ptr>::iterator it = _endpoints.begin();
	while ( it->first < query->eid() && it != _endpoints.end() ) { it++; }
	while ( it->first < ( query->eid() + 32 ) && it != _endpoints.end() )
	{
		group |= (1 << (it->first - query->eid()) );
		it++;
	}

	try {
		socket->send(InfoPacket<uint32_t>(query->eid(), group), from);
	} catch ( const NetworkException& error ) {
		std::stringstream oss;
		oss << "An error occured when replying to an EP group query packet: " << error.reason() << ": " << error.code().message();
		_asyncError(GenericException(oss.str()));
	}
}

void Device::_handle_descepquery(const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	for ( std::vector<hexabus::Socket*>::iterator it = _sockets.begin(), end = _sockets.end(); it != end; ++it )
	{
		_handle_descepquery(*it, p, from);
	}
}

void Device::_handle_descepquery(hexabus::Socket* socket, const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const EIDPacket* query = dynamic_cast<const EIDPacket*>(&p);
	if ( !query ) {
		_asyncError(GenericException("Trying to handle a device description query but didn't get an EIDPacket"));
		return;
	}

	std::string device_name = "";
	if ( _read.num_slots() > 0 )
		device_name = *_read();

	try {
		socket->send(EndpointInfoPacket(query->eid(), HXB_DTYPE_UINT32, device_name), from);
	} catch ( const NetworkException& error ) {
		std::stringstream oss;
		oss << "An error occured when replying to a device description query packet from " << from << ": " << error.reason() << ": " << error.code().message();
		_asyncError(GenericException(oss.str()));
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

void Device::_handle_smupload(hexabus::Socket* socket, const Packet& p, const boost::asio::ip::udp::endpoint& from)
{
	const WritePacket<boost::array<uint8_t, 65> >* write = dynamic_cast<const WritePacket<boost::array<uint8_t, 65> >*>(&p);
	if ( !write ) {
		try {
			socket->send(InfoPacket<bool>(EP_SM_UP_ACKNAK, false), from);
		} catch ( const NetworkException& error ) {
			std::stringstream oss;
			oss << "An error occured when sending a NAK in reply to a malformed statemachine upload: " << error.reason() << ": " << error.code().message();
			_asyncError(GenericException(oss.str()));
		}
		return;
	}
	const boost::array<uint8_t, 65> data = write->value();
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
			try {
				_write(name);
			} catch ( const GenericException& error ) {
				std::stringstream oss;
				oss << "An error occured when setting the device name: " << error.reason();
				_asyncError(GenericException(oss.str()));
			}
		}
	}
	try {
		socket->send(InfoPacket<bool>(EP_SM_UP_ACKNAK, true), from);
	} catch ( const NetworkException& error ) {
		std::stringstream oss;
		oss << "An error occured when sending an ACK in reply to a valid statemachine upload: " << error.reason() << ": " << error.code().message();
		_asyncError(GenericException(oss.str()));
	}
}

void Device::_handle_broadcasts(const boost::system::error_code& error)
{
	if ( !error )
	{
		for ( std::map<uint32_t, const EndpointFunctions::Ptr>::iterator epIt = _endpoints.begin(), end = _endpoints.end(); epIt != end; ++epIt )
		{
			if ( !epIt->second->is_readable() || !epIt->second->broadcast() )
				continue;

			try {
				Packet::Ptr p = epIt->second->handle_query();
				if ( p && p->type() != HXB_PTYPE_ERROR )
				{
					for ( std::vector<hexabus::Socket*>::const_iterator sIt = _sockets.begin(), sEnd = _sockets.end(); sIt != sEnd; ++sIt )
					{
						try {
							(*sIt)->send(*p);
						} catch ( const NetworkException& error ) {
							std::stringstream oss;
							oss << "An error occured when broadcasting endpoint " << epIt->first << ": " << error.reason() << ": " << error.code().message();
							_asyncError(GenericException(oss.str()));
						}
					}
				} else {
					std::stringstream oss;
					oss << "Unable to generate broadcast packet";
					if (p) {
						ErrorPacket* pe = (ErrorPacket*) p.get();
						oss << " (" << (int) pe->code() << ")";
					}
					throw hexabus::GenericException(oss.str());
				}
			} catch ( const GenericException& error ) {
				std::stringstream oss;
				oss << "An error occured when reading endpoint " << epIt->first << ": " << error.reason();
				_asyncError(GenericException(oss.str()));
			}
		}

		_timer.expires_from_now(boost::posix_time::seconds(_interval+(rand()%_interval)));
		_timer.async_wait(boost::bind(&Device::_handle_broadcasts, this, _1));
	} else {
		std::cerr << "handle_broadcast boost-error was set."  << std::endl;
	}
}

void Device::_handle_errors(const GenericException& error)
{
	_asyncError(error);
}

