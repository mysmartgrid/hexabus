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
          const double& min_value,
          const double& max_value)
        : _sensor_id(sensor_id)
          , _sensor_name(sensor_name)
          , _min_value(min_value)
          , _max_value(max_value)
      {};
      virtual ~Sensor() {};

      void put( http::client client,
          const uri::uri& api_uri,
          const double& reading);
      void post_value(http::client client,
          const uri::uri& api_uri, 
          const double& reading);

    private:
      Sensor (const Sensor& original);
      Sensor& operator= (const Sensor& rhs);

      std::string _sensor_id;
      std::string _sensor_name;
      double _min_value;
      double _max_value;
  };
};


#endif /* LIBHEXANODE_SENSOR_HPP */

