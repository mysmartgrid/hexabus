#include "libhexabus/endpoint_registry.hpp"

#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "private/paths.hpp"
#include "error.hpp"

using namespace hexabus;

const char* EndpointRegistry::default_path = LIBHEXABUS_INSTALL_PREFIX "/share/libhexabus/endpoint_registry";

static boost::filesystem::path path_from_env_or_default()
{
	const char* from_env = std::getenv("HXB_ENDPOINT_REGISTRY");
	if (from_env) {
		return from_env;
	} else {
		return EndpointRegistry::default_path;
	}
}

EndpointRegistry::EndpointRegistry()
	: _path(path_from_env_or_default())
{
	reload();
}

EndpointRegistry::EndpointRegistry(const boost::filesystem::path& path)
	: _path(path)
{
	reload();
}

static std::string single_child(const boost::property_tree::ptree& tree, const std::string& key, uint32_t eid)
{
	switch (tree.count(key)) {
		case 0:
			{
				std::ostringstream o;
				o << "Key '" << key << "' not found for EID " << eid;
				throw GenericException(o.str());
			}
		
		case 1:
			break;

		default:
			{
				std::ostringstream o;
				o << "Duplicate key '" << key << "' for EID" << eid;
				throw GenericException(o.str());
			}
	}

	return tree.find(key)->second.get_value<std::string>();
}

void EndpointRegistry::reload()
{
	boost::filesystem::ifstream file(_path, std::ios_base::in);

	if (!file.good())
		throw GenericException("Endpoint registry file not found");

	boost::property_tree::ptree ptree;

	try {
		read_info(file, ptree);
	} catch (const boost::property_tree::info_parser_error& e) {
		throw GenericException(e.what());
	}

	std::map<uint32_t, EndpointDescriptor> eids;

	typedef boost::property_tree::ptree::const_iterator iterator;
	for (iterator it = ptree.begin(), end = ptree.end(); it != end; it++) {
		if (it->first != "eid") {
			throw GenericException("Invalid endpoint registry file");
		}

		uint32_t eid;
		std::string description;
		boost::optional<std::string> unit;
		std::string access_str;
		EndpointDescriptor::Access access;
		EndpointDescriptor::Function function;
		std::string type_str;
		std::string function_str;
		hxb_datatype type;

		try {
			eid = it->second.get_value<uint32_t>();
		} catch (...) {
			std::ostringstream o;
			o << "Invalid EID " << it->second.get_value<std::string>();
			throw GenericException(o.str());
		}

		if (eids.count(eid)) {
			std::ostringstream o;
			o << "Duplicate descriptors for EID " << eid;
			throw GenericException(o.str());
		}

		description = single_child(it->second, "description", eid);
		unit = it->second.get_optional<std::string>("unit");

		type_str = single_child(it->second, "type", eid);
		if (boost::equals(type_str, "BOOL"))
			type = HXB_DTYPE_BOOL;
		else if (boost::equals(type_str, "UINT8"))
			type = HXB_DTYPE_UINT8;
		else if (boost::equals(type_str, "UINT32"))
			type = HXB_DTYPE_UINT32;
		else if (boost::equals(type_str, "DATETIME"))
			type = HXB_DTYPE_DATETIME;
		else if (boost::equals(type_str, "FLOAT"))
			type = HXB_DTYPE_FLOAT;
		else if (boost::equals(type_str, "128STRING"))
			type = HXB_DTYPE_128STRING;
		else if (boost::equals(type_str, "TIMESTAMP"))
			type = HXB_DTYPE_TIMESTAMP;
		else if (boost::equals(type_str, "65BYTES"))
			type = HXB_DTYPE_65BYTES;
		else if (boost::equals(type_str, "16BYTES"))
			type = HXB_DTYPE_16BYTES;
		else {
			std::ostringstream o;
			o << "Invalid type " << type_str << " for EID " << eid;
			throw GenericException(o.str());
		}

		function_str = single_child(it->second, "function", eid);
		if (boost::equals(function_str, "sensor"))
			function = EndpointDescriptor::sensor;
		else if (boost::equals(function_str, "actor"))
			function = EndpointDescriptor::actor;
		else if (boost::equals(function_str, "infrastructure"))
			function = EndpointDescriptor::infrastructure;

		access_str = single_child(it->second, "access", eid);
		if (boost::equals(access_str, "R"))
			access = EndpointDescriptor::read;
		else if (boost::equals(access_str, "W"))
			access = EndpointDescriptor::write;
		else if (boost::equals(access_str, "RW"))
			access = EndpointDescriptor::read | EndpointDescriptor::write;
		else {
			std::ostringstream o;
			o << "Invalid access " << access_str << " for EID " << eid;
			throw GenericException(o.str());
		}

		eids.insert(std::make_pair(eid, EndpointDescriptor(eid, description, unit, type, access, function)));
	}

	_eids = eids;
}

EndpointRegistry::const_iterator EndpointRegistry::begin() const
{
	return _eids.begin();
}

EndpointRegistry::const_iterator EndpointRegistry::end() const
{
	return _eids.end();
}

EndpointRegistry::const_iterator EndpointRegistry::find(uint32_t eid) const
{
	return _eids.find(eid);
}

const EndpointDescriptor& EndpointRegistry::lookup(uint32_t eid) const
{
	const_iterator found = find(eid);

	if (found == end())
		throw std::out_of_range("eid");

	return found->second;
}
