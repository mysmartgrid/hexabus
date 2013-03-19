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

void Sensor::put(
    http::client client,
    const uri::uri& api_uri, 
    const std::string& reading)
{
  std::cout << "Creating sensor " << _sensor_name << std::endl;
    StringBuffer b;
  PrettyWriter<StringBuffer> writer(b);
  writer.StartObject();
  writer.String("name");
  writer.String(_sensor_name.c_str(), (SizeType) _sensor_name.length());
  writer.String("value");
  writer.String(reading.c_str(), (SizeType) reading.length());
  writer.String("minvalue");
  writer.String(_min_value.c_str(), (SizeType) _min_value.length());
  writer.String("maxvalue");
  writer.String(_max_value.c_str(), (SizeType) _max_value.length());
  writer.EndObject();
  std::string json_body(b.GetString());

  uri::uri create;
  create << api_uri 
    << uri::path("/sensor/") 
    << uri::path(uri::encoded(_sensor_id));
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
  writer.String("value");
  writer.String(reading.c_str(), (SizeType) reading.length());
  writer.EndObject();
  std::string json_body(b.GetString());

  uri::uri create;
  create << api_uri 
    << uri::path("/sensor/") 
    << uri::path(uri::encoded(_sensor_id));
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
