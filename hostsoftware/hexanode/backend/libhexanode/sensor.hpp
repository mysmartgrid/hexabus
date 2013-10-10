#ifndef LIBHEXANODE_SENSOR_HPP
#define LIBHEXANODE_SENSOR_HPP 1

#include <libhexanode/common.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>
#include <libhexabus/endpoint_registry.hpp>

using namespace boost::network;

namespace hexanode {
  class Sensor {
    public:
      typedef boost::shared_ptr<Sensor> Ptr;
      Sensor(const boost::asio::ip::address_v6& sensor_ip,
					const hexabus::EndpointDescriptor& ep_info,
          const std::string& sensor_name,
          const int32_t min_value,
          const int32_t max_value,
          const std::string& type)
        : _sensor_ip(sensor_ip)
					, _ep_info(ep_info)
          , _sensor_name(sensor_name)
          , _min_value(min_value)
          , _max_value(max_value)
          , _type(type)
      {};
      virtual ~Sensor() {};

      void put( http::client client,
          const uri::uri& api_uri,
          const std::string& reading);
      void post_value(http::client client,
          const uri::uri& api_uri, 
          const std::string& reading);

    private:
			boost::asio::ip::address_v6 _sensor_ip;
			hexabus::EndpointDescriptor _ep_info;
      std::string _sensor_name;
      int32_t _min_value;
      int32_t _max_value;
      std::string _type;
  };
};


#endif /* LIBHEXANODE_SENSOR_HPP */

