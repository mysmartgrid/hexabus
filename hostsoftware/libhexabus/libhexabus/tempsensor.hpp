#ifndef LIBHEXABUS_TEMPSENSOR_HPP
#define LIBHEXABUS_TEMPSENSOR_HPP 1

#include <libhexabus/common.hpp>
#include <libhexabus/sensor.hpp>
#include <iostream>

namespace hexabus {
  class TempSensor : public Sensor {
    public:
      typedef std::tr1::shared_ptr<TempSensor> Ptr;
      TempSensor (const std::string& id)
        : Sensor(id) {};
      const std::string str() const;
      void add_value(float value);
      void save_values(std::ostream& os);
      ~TempSensor() {};

    private:
      TempSensor (const TempSensor& original);
      TempSensor& operator= (const TempSensor& rhs);

  };
};


#endif /* LIBHEXABUS_TEMPSENSOR_HPP */

