#ifndef LIBHEXANODE_SENSOR_HPP
#define LIBHEXANODE_SENSOR_HPP 1

#include <libhexanode/common.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>

using namespace boost::network;

namespace hexanode {
  class Sensor {
    public:
      typedef boost::shared_ptr<Sensor> Ptr;
      Sensor(const std::string& sensor_id,
          const std::string& sensor_name,
          const int32_t min_value,
          const int32_t max_value,
          const std::string& type)
        : _sensor_id(sensor_id)
          , _sensor_name(sensor_name)
          , _min_value(min_value)
          , _max_value(max_value)
          , _displaytype(type)
      {};
      virtual ~Sensor() {};

      const std::string& get_id() { return _sensor_id; };
      void put( http::client client,
          const uri::uri& api_uri,
          const std::string& reading);
      void post_value(http::client client,
          const uri::uri& api_uri, 
          const std::string& reading);

    private:
      Sensor (const Sensor& original);
      Sensor& operator= (const Sensor& rhs);

      std::string _sensor_id;
      std::string _sensor_name;
      int32_t _min_value;
      int32_t _max_value;
      std::string _displaytype;
  };
};


#endif /* LIBHEXANODE_SENSOR_HPP */

