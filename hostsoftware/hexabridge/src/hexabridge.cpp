/**
 * This file is part of hexabridge.
 *
 * (c) Fraunhofer ITWM - Stephan Platz <platz@itwm.fhg.de>, 2014
 *
 * hexabridge is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hexabridge is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hexabridge. If not, see <http://www.gnu.org/licenses/>.
 */
#include "hexabridge.hpp"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/ref.hpp>

#include <libhexabus/device.hpp>
#include <libhexabus/endpoint_registry.hpp>

#include <json/json.h>
#include <libmysmartgrid/error.h>
#include <libmysmartgrid/webclient.h>

using namespace hexabridge;

namespace bf = boost::filesystem;

std::string _device_name = "HexaBridge";
uint32_t eid = 2;

Pusher::Pusher(boost::asio::io_service& io, const std::vector<std::string>& interfaces, const std::vector<std::string>& addresses,
		const std::string& url, const std::string& id, const std::string& token, int interval, bool debug)
	: _device(io, interfaces, addresses, interval)
	, _url(url)
	, _id(id)
	, _token(token)
	, _debug(debug)
{
	_init();
}

void Pusher::_init() {
	_device.onReadName(boost::bind(&Pusher::loadDeviceName, this));
	_device.onWriteName(boost::bind(&Pusher::saveDeviceName, this, _1));

	hexabus::EndpointRegistry ep_registry;
	hexabus::EndpointRegistry::const_iterator ep_it;

	ep_it = ep_registry.find(eid);
	hexabus::TypedEndpointFunctions<uint32_t>::Ptr powerEP = ep_it != ep_registry.end()
		? hexabus::TypedEndpointFunctions<uint32_t>::fromEndpointDescriptor(ep_it->second)
		: hexabus::TypedEndpointFunctions<uint32_t>::Ptr(new hexabus::TypedEndpointFunctions<uint32_t>(eid, "HexabusPlug+ Power meter (W)"));
	powerEP->onRead(boost::bind(&Pusher::getLastReading, this));
	_device.addEndpoint(powerEP);
}

uint32_t Pusher::getLastReading() {
	std::stringstream err;
	// Try multiple times to query mysmartgrid
	for ( unsigned int i = 0; i < 5; ++i ) {
		try {
			libmsg::Webclient::Reading reading = libmsg::Webclient::getLastReading(_url, _id, _token);
			if ( reading.second > 0 )
				return reading.second;
		} catch ( const libmsg::CommunicationException& e ) {
			// ignore network problems
			err << "An error occured while communicating with the mysmartgrid server: " << e.what();
		}
	}
	throw libmsg::CommunicationException(err.str());
}

std::string Pusher::loadDeviceName()
{
	_debug && std::cout << "loading device name" << std::endl;
	std::string name;
	name = _device_name;

	return name;
}

void Pusher::saveDeviceName(const std::string& name)
{
	_debug && std::cout << "saving device name \"" << name << "\"" << std::endl;
	if ( name.size() > 30 )
	{
		std::cerr << "cannot save a device name of length > 30" << std::endl;
		return;
	}

	_device_name = name;
}
