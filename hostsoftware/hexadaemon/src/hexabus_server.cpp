#include "hexabus_server.hpp"

#include <syslog.h>

#include <fstream>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/ref.hpp>

#include <libhexabus/device.hpp>
#include <libhexabus/endpoint_registry.hpp>
#include "../../../shared/hexabus_definitions.h"

#include "endpoints.h"
#include "configure.h"

#ifdef UCI_FOUND
extern "C" {
#include <uci.h>
}
#endif

using namespace hexadaemon;

namespace bf = boost::filesystem;

#ifndef UCI_FOUND
	std::string _device_name = "Hexadaemon";
#endif /* UCI_FOUND */

HexabusServer::HexabusServer(boost::asio::io_service& io, const std::vector<std::string>& interfaces, const std::vector<std::string>& addresses, int interval, bool debug)
	: _device(io, interfaces, addresses, interval)
	, _debug(debug)
{
	_init();
}

void HexabusServer::_init() {
	loadSensorMapping();

	_device.onReadName(boost::bind(&HexabusServer::loadDeviceName, this));
	_device.onWriteName(boost::bind(&HexabusServer::saveDeviceName, this, _1));
	_device.onAsyncError(boost::bind(&HexabusServer::handleAsyncError, this, _1));

	hexabus::EndpointRegistry ep_registry;
	hexabus::EndpointRegistry::const_iterator ep_it;

	ep_it = ep_registry.find(EP_POWER_METER);
	hexabus::TypedEndpointFunctions<uint32_t>::Ptr powerEP = ep_it != ep_registry.end()
		? hexabus::TypedEndpointFunctions<uint32_t>::fromEndpointDescriptor(ep_it->second)
		: hexabus::TypedEndpointFunctions<uint32_t>::Ptr(new hexabus::TypedEndpointFunctions<uint32_t>(EP_POWER_METER, "HexabusPlug+ Power meter (W)"));
	powerEP->onRead(boost::bind(&HexabusServer::get_sum, this));
	_device.addEndpoint(powerEP);

	ep_it = ep_registry.find(EP_FLUKSO_L1);
	hexabus::TypedEndpointFunctions<uint32_t>::Ptr l1EP = ep_it != ep_registry.end()
		? hexabus::TypedEndpointFunctions<uint32_t>::fromEndpointDescriptor(ep_it->second)
		: hexabus::TypedEndpointFunctions<uint32_t>::Ptr(new hexabus::TypedEndpointFunctions<uint32_t>(EP_FLUKSO_L1, "Flukso Phase 1"));
	l1EP->onRead(boost::bind(&HexabusServer::get_sensor, this, 1));
	_device.addEndpoint(l1EP);

	ep_it = ep_registry.find(EP_FLUKSO_L2);
	hexabus::TypedEndpointFunctions<uint32_t>::Ptr l2EP = ep_it != ep_registry.end()
		? hexabus::TypedEndpointFunctions<uint32_t>::fromEndpointDescriptor(ep_it->second)
		: hexabus::TypedEndpointFunctions<uint32_t>::Ptr(new hexabus::TypedEndpointFunctions<uint32_t>(EP_FLUKSO_L2, "Flukso Phase 2"));
	l2EP->onRead(boost::bind(&HexabusServer::get_sensor, this, 2));
	_device.addEndpoint(l2EP);

	ep_it = ep_registry.find(EP_FLUKSO_L3);
	hexabus::TypedEndpointFunctions<uint32_t>::Ptr l3EP = ep_it != ep_registry.end()
		? hexabus::TypedEndpointFunctions<uint32_t>::fromEndpointDescriptor(ep_it->second)
		: hexabus::TypedEndpointFunctions<uint32_t>::Ptr(new hexabus::TypedEndpointFunctions<uint32_t>(EP_FLUKSO_L3, "Flukso Phase 3"));
	l3EP->onRead(boost::bind(&HexabusServer::get_sensor, this, 3));
	_device.addEndpoint(l3EP);

	ep_it = ep_registry.find(EP_FLUKSO_S01);
	hexabus::TypedEndpointFunctions<uint32_t>::Ptr l4EP = ep_it != ep_registry.end()
		? hexabus::TypedEndpointFunctions<uint32_t>::fromEndpointDescriptor(ep_it->second)
		: hexabus::TypedEndpointFunctions<uint32_t>::Ptr(new hexabus::TypedEndpointFunctions<uint32_t>(EP_FLUKSO_S01, "Flukso S0 1"));
	l4EP->onRead(boost::bind(&HexabusServer::get_sensor, this, 4));
	_device.addEndpoint(l4EP);

	ep_it = ep_registry.find(EP_FLUKSO_S02);
	hexabus::TypedEndpointFunctions<uint32_t>::Ptr l5EP = ep_it != ep_registry.end()
		? hexabus::TypedEndpointFunctions<uint32_t>::fromEndpointDescriptor(ep_it->second)
		: hexabus::TypedEndpointFunctions<uint32_t>::Ptr(new hexabus::TypedEndpointFunctions<uint32_t>(EP_FLUKSO_S02, "Flukso S0 2"));
	l5EP->onRead(boost::bind(&HexabusServer::get_sensor, this, 5));
	_device.addEndpoint(l5EP);
}

static const char* entry_names[6] = {
	0,
	"Phase 1",
	"Phase 2",
	"Phase 3",
	"S0 1",
	"S0 2",
};

unsigned long endpoints[6] = {
	0, // Dummy value as there is no sensor 0
	EP_FLUKSO_L1,
	EP_FLUKSO_L2,
	EP_FLUKSO_L3,
	EP_FLUKSO_S01,
	EP_FLUKSO_S02,
};

uint32_t HexabusServer::get_sensor(int map_idx)
{
	updateFluksoValues();
	_debug && std::cout << "Reading value for " << entry_names[map_idx] << std::endl;
	return _flukso_values[_sensor_mapping[map_idx]];
}

uint32_t HexabusServer::get_sum()
{
	updateFluksoValues();
	int result = 0;

	result += _flukso_values[_sensor_mapping[1]];
	result += _flukso_values[_sensor_mapping[2]];
	result += _flukso_values[_sensor_mapping[3]];

	return result;
}

void HexabusServer::updateFluksoValues()
{
	bf::path p("/var/run/fluksod/sensor/");

	if ( exists(p) && is_directory(p) )
	{
		for ( bf::directory_iterator sensors(p); sensors != bf::directory_iterator(); sensors++ )
		{
			std::string filename = (*sensors).path().filename().string();
			boost::regex hex32("^[0-9a-f]{32}$");
			boost::match_results<std::string::const_iterator> what;
			if ( !boost::regex_match(filename, what, hex32) ) {
				_debug && std::cout << "Ignoring file: " << filename << std::endl;
				continue;
			}

			_debug && std::cout << "Parsing file: " << filename << std::endl;

			std::ifstream file;
			file.open((*sensors).path().string().c_str());
			if ( file.fail() )
				continue;

			std::string flukso_data;
			file >> flukso_data;
			file.close();
			//extract last value != "nan" from the json array
			boost::regex r("^\\[(?:\\[[[:digit:]]*,[[:digit:]]*\\],)*\\[[[:digit:]]*,([[:digit:]]*)\\](?:,\\[[[:digit:]]*,\"nan\"\\])*\\]$");

			if ( boost::regex_search(flukso_data, what, r) ) {
				try {
					uint32_t value = boost::lexical_cast<uint32_t>(std::string(what[1].first, what[1].second));
					_flukso_values[filename] = value;
					_debug && std::cout << "Updating _flukso_values[" << filename << "] = " << value << std::endl;
				} catch ( const boost::bad_lexical_cast & ) {
					std::cerr << "Error parsing value " << std::string(what[1].first, what[1].second) << std::endl;
				}
			} else {
        boost::regex r0("^\\[(?:\\[[[:digit:]]*,\"nan\"\\],)*\\[[[:digit:]]*,\"nan\"\\]\\]$");
        if ( boost::regex_search(flukso_data, what, r0) ) {
          _debug && std::cerr << "No Values " << filename << std::endl;
					_flukso_values[filename] = 0;
				} else {
          std::cerr << "Error parsing " << filename << std::endl;
          _debug && std::cout << "Content of " << filename << ": \'" << flukso_data << "\'" << std::endl;
        }
			}
		}
	}
}

void HexabusServer::loadSensorMapping()
{
	_debug && std::cout << "loading sensor mapping" << std::endl;
#ifdef UCI_FOUND
	uci_context *ctx = uci_alloc_context();
	uci_package *flukso;
	uci_load(ctx, "flukso", &flukso);
	for ( unsigned int i = 1; i < 6; i++ )
	{
		int port;
		std::string id;
		struct uci_ptr res;
		char portKey[14], idKey[12];
		snprintf(portKey, 14, "flukso.%d.port", i);
		snprintf(idKey, 12, "flukso.%d.id", i);
		_debug && std::cout << "Looking up " << portKey << std::endl;
		if (uci_lookup_ptr(ctx, &res, portKey, true) != UCI_OK)
		{
			std::cerr << "Port lookup " << portKey << " failed!" << std::endl;
			uci_perror(ctx, "hexadaemon");
			continue;
		}
		if (!(res.flags & uci_ptr::UCI_LOOKUP_COMPLETE)) {
			std::cerr << "Unable to load sensor port " << portKey << " configuration" << std::endl;
			continue;
		}
		if (res.last->type != UCI_TYPE_OPTION) {
			std::cerr << "Looking up sensor port " << i << " configuration failed, not an option" << std::endl;
			continue;
		}
		if (res.o->type == UCI_TYPE_LIST) {
			port = atoi(list_to_element(res.o->v.list.next)->name);
		} else {
			port = atoi(res.o->v.string);
		}
		_debug && std::cout << "Port: " << port << std::endl;
		_debug && std::cout << "Looking up " << idKey << std::endl;
		if (uci_lookup_ptr(ctx, &res, idKey, true) != UCI_OK)
		{
			std::cerr << "Id lookup " << idKey << " failed!" << std::endl;
			uci_perror(ctx, "hexadaemon");
			continue;
		}
		if (!(res.flags & uci_ptr::UCI_LOOKUP_COMPLETE)) {
			std::cerr << "Unable to load sensor id " << idKey << " configuration" << std::endl;
			continue;
		}
		if (res.last->type != UCI_TYPE_OPTION) {
			std::cerr << "Looking up sensor id " << i << " configuration failed, not an option" << std::endl;
			continue;
		}
		if (res.o->type != UCI_TYPE_STRING) {
			std::cerr << "Looking up sensor id " << i << " configuration failed, not a string but " << res.o->type << std::endl;
			continue;
		}
		_debug && std::cout << "Id: " << res.o->v.string << std::endl;
		id = res.o->v.string;
		_sensor_mapping[port] = id;
	}
	uci_free_context(ctx);
#else /* UCI_FOUND */
	_debug && std::cout << "UCI disabled. NO sensors loaded!" << std::endl;
#endif /* UCI_FOUND */
	_debug && std::cout << "sensor mapping loaded" << std::endl;
}

std::string HexabusServer::loadDeviceName()
{
	_debug && std::cout << "loading device name" << std::endl;
	std::string name;
#ifdef UCI_FOUND
	uci_context *ctx = uci_alloc_context();
	uci_package *flukso;
	uci_load(ctx, "flukso", &flukso);
	char key[18] = "hexabus.main.name";
	struct uci_ptr res;
	if (uci_lookup_ptr(ctx, &res, key, true) != UCI_OK)
	{
		std::cerr << "Name lookup failed!" << std::endl;
		uci_perror(ctx, "hexadaemon");
		return "";
	}
	if (!(res.flags & uci_ptr::UCI_LOOKUP_COMPLETE)) {
		std::cerr << "Unable to load name" << std::endl;
		return "";
	}
	if (res.last->type != UCI_TYPE_OPTION) {
		std::cerr << "Looking up device name failed, not an option" << std::endl;
		return "";
	}
	if (res.o->type != UCI_TYPE_STRING) {
		std::cerr << "Looking up device name failed, not a string but " << res.o->type << std::endl;
		return "";
	}
	name = res.o->v.string;

	uci_free_context(ctx);
	_debug && std::cout << "device name \"" << name << "\" loaded" << std::endl;
#else /* UCI_FOUND */
	name = _device_name;
#endif /* UCI_FOUND */

	return name;
}

void HexabusServer::saveDeviceName(const std::string& name)
{
	_debug && std::cout << "saving device name \"" << name << "\"" << std::endl;
	if ( name.size() > 30 )
	{
		std::cerr << "cannot save a device name of length > 30" << std::endl;
		return;
	}

#ifdef UCI_FOUND
	uci_context *ctx = uci_alloc_context();
	uci_package *flukso;
	uci_load(ctx, "flukso", &flukso);
	char key[49];
	snprintf(key, 49, "hexabus.main.name=%s", name.c_str());
	struct uci_ptr res;
	if (uci_lookup_ptr(ctx, &res, key, true) != UCI_OK)
	{
		std::cerr << "Name lookup failed!" << std::endl;
		uci_perror(ctx, "hexadaemon");
		return;
	}
	if (uci_set(ctx, &res) != UCI_OK)
	{
		std::cerr << "Unable to set name!" << std::endl;
		return;
	}
	if (uci_save(ctx, res.p) != UCI_OK)
	{
		std::cerr << "Unable to save name!" << std::endl;
		return;
	}
	if (uci_commit(ctx, &res.p, false) != UCI_OK)
	{
		std::cerr << "Unable to commit name!" << std::endl;
		return;
	}

	uci_free_context(ctx);
#else /* UCI_FOUND */
	_device_name = name;
#endif /* UCI_FOUND */
}

void HexabusServer::handleAsyncError(const hexabus::GenericException& error)
{
	_debug && std::cerr << "Asynchronous error occured: " << error.reason() << std::endl;
}
