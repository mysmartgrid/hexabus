#include "sensor.hpp"
#include <boost/network/uri/uri_io.hpp>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <libhexanode/error.hpp>

using namespace boost::network;
using namespace rapidjson;

using namespace hexanode;

namespace {

void jstring(PrettyWriter<StringBuffer>& target, const char* key, const std::string& value)
{
	target.String(key);
	target.String(value.c_str(), value.size());
}

void jint(PrettyWriter<StringBuffer>& target, const char* key, int value)
{
	target.String(key);
	target.Int(value);
}

}

void Sensor::put(
    http::client client,
    const uri::uri& api_uri, 
    const std::string& reading)
{
  std::cout << "Creating sensor " << _sensor_name << std::endl;
    StringBuffer b;
  PrettyWriter<StringBuffer> writer(b);
  writer.StartObject();
	jstring(writer, "name", _sensor_name);
	if (_ep_info.unit() && *_ep_info.unit() == "degC") {
		jstring(writer, "unit", "°C");
	} else {
		jstring(writer, "unit", _ep_info.unit().get_value_or(""));
	}
	jstring(writer, "description", _ep_info.description());
	switch (_ep_info.function()) {
		case hexabus::EndpointDescriptor::sensor:
			jstring(writer, "function", "sensor");
			break;

		case hexabus::EndpointDescriptor::actor:
			jstring(writer, "function", "actor");
			break;

		case hexabus::EndpointDescriptor::infrastructure:
			jstring(writer, "function", "infrastructure");
			break;

		default:
			throw hexanode::GenericException("unknown function");
	}
	jstring(writer, "value", reading);
	jint(writer, "minvalue", _min_value);
	jint(writer, "maxvalue", _max_value);
	jstring(writer, "type", _type);
  writer.EndObject();
  std::string json_body(b.GetString());

  uri::uri create;
  create << api_uri 
    << uri::path("/sensor/") 
		<< uri::path(uri::encoded(_sensor_ip.to_string()))
		<< uri::path("/")
		<< uri::path(uri::encoded(boost::lexical_cast<std::string>(_ep_info.eid())));
  http::client::request request(create);
  request << header("Content-Type", "application/json");
  std::ostringstream converter;
  converter << json_body.length();
  request << header("Content-Length", converter.str());
  request << body(json_body);
  http::client::response response = client.put(request);
  if (response.status() != 200) {
    std::ostringstream oss;
    oss << response.status() 
    << " " << response.status_message() 
    << ": " << response.body();
    throw hexanode::CommunicationException(oss.str());
  }

}

void Sensor::post_value(
    http::client client,
    const uri::uri& api_uri, 
    const std::string& reading)
{
  //std::cout << "Pushing value "<< reading 
  //  << " to sensor " << sensor_id << std::endl;
  StringBuffer b;
  PrettyWriter<StringBuffer> writer(b);
  writer.StartObject();
	jstring(writer, "value", reading);
  writer.EndObject();
  std::string json_body(b.GetString());

  uri::uri create;
  create << api_uri 
    << uri::path("/sensor/") 
		<< uri::path(uri::encoded(_sensor_ip.to_string()))
		<< uri::path("/")
		<< uri::path(uri::encoded(boost::lexical_cast<std::string>(_ep_info.eid())));
  http::client::request request(create);
  request << header("Content-Type", "application/json");
  std::ostringstream converter;
  converter << json_body.length();
  request << header("Content-Length", converter.str());
  request << body(json_body);
  http::client::response response = client.post(request);
  if (response.status() != 200) {
    std::ostringstream oss;
    oss << response.status() 
    << " " << response.status_message() 
    << ": " << response.body();
    throw hexanode::CommunicationException(oss.str());
  }
}
