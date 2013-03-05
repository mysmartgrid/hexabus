#include <libhexanode/common.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>
#include <boost/network/uri/uri_io.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/thread/thread.hpp>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;
#include <iostream>
#include <sstream>

using namespace boost::network;
using namespace rapidjson;

void mk_sensor(
    http::client client,
    const uri::uri& api_uri, 
    const std::string sensor_id,
    const std::string sensor_name,
    const std::string min_value,
    const std::string max_value,
    const std::string reading)
{
  std::cout << "Creating sensor " << sensor_name << std::endl;
    StringBuffer b;
  PrettyWriter<StringBuffer> writer(b);
  writer.StartObject();
  writer.String("name");
  writer.String(sensor_name.c_str(), (SizeType) sensor_name.length());
  writer.String("value");
  writer.String(reading.c_str(), (SizeType) reading.length());
  writer.String("minvalue");
  writer.String(min_value.c_str(), (SizeType) min_value.length());
  writer.String("maxvalue");
  writer.String(max_value.c_str(), (SizeType) max_value.length());
  writer.EndObject();
  std::string json_body(b.GetString());

  uri::uri create;
  create << api_uri 
    << uri::path("/sensor/") 
    << uri::path(uri::encoded(sensor_id));
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
    throw std::runtime_error(oss.str());
  }
}

void push_value(
    http::client client,
    const uri::uri& api_uri, 
    const std::string& sensor_id,
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
    << uri::path(uri::encoded(sensor_id));
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
    throw std::runtime_error(oss.str());
  }
}

int main(int argc, char *argv[]) {
  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " IP [additional options] ACTION";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexanode version and exit")
    ("frontendurl,u", po::value<std::string>(), "URL of frontend API")
    ;
  po::positional_options_description p;
  p.add("frontendurl", 1);
  po::variables_map vm;

  // Begin processing of commandline parameters.
  try {
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
    exit(-1);
  }

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  if (vm.count("version")) {
    hexanode::VersionInfo v;
    std::cout << "hexanode demo data generator" << std::endl;
    std::cout << "hexanode version " << v.get_version() << std::endl;
    return 0;
  }

  uri::uri base_uri;

  if (vm.count("frontendurl") != 1) {
    std::cerr << "No frontend URL given - exiting." << std::endl;
    return 1;
  } else {
    base_uri << uri::scheme("http") 
      << uri::host(vm["frontendurl"].as<std::string>())
      << uri::path("/api");
  }

  std::cout << "Using frontend url " << base_uri << std::endl;

  http::client client;
  uint16_t max_id = 4;
  boost::random::mt19937 gen;
  boost::random::uniform_int_distribution<> dist(15, 30);

  std::cout << "Will now push values." << std::endl;
  while(true) {
    try {
      for (uint16_t sid=0; sid < max_id; ++sid) {
        std::ostringstream idconv;
        std::ostringstream valconv;
        idconv << sid;
        valconv << std::fixed << std::setprecision(2);
        valconv << dist(gen);
        push_value(client, base_uri, idconv.str(), valconv.str());
      }
    } catch (const std::exception& e) {
      std::cout << "Cannot submit sensor values (" << e.what() << ")" << std::endl;
      std::cout << "Attempting to re-create sensors." << std::endl;
      mk_sensor(client, base_uri, "0", "Temperatur Flur 1", "15", "30", "0"); 
      mk_sensor(client, base_uri, "1", "Temperatur Buero MD", "15", "30", "0"); 
      mk_sensor(client, base_uri, "2", "Labor", "15", "30", "0"); 
      mk_sensor(client, base_uri, "3", "Temperatur Buero 2", "15", "30", "0"); 
      continue; // do not sleep
    }

    boost::this_thread::sleep( boost::posix_time::seconds(3) );
  }
}
