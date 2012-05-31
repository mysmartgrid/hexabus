#ifndef LIBHEXABUS_SENSOR_HPP
#define LIBHEXABUS_SENSOR_HPP 1

#include <libhexabus/common.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <map>

namespace hexabus {

  class Sensor {
    public:
      typedef std::tr1::shared_ptr<Sensor> Ptr;
      Sensor (const std::string& id) 
        : _id(id), _values() {};
      virtual const std::string str() const = 0;
      virtual const std::string get_id() const;
      virtual void add_value(float value) = 0;
      virtual void save_values(std::ostream& os) = 0;
      virtual ~Sensor() {};

    protected:
      const boost::posix_time::ptime get_timestamp() const;

      std::string _id;
      std::map<boost::posix_time::ptime, float> _values;

    private:
      Sensor (const Sensor& original);
      Sensor& operator= (const Sensor& rhs);
  };

}

#endif /* LIBHEXABUS_SENSOR_HPP */

