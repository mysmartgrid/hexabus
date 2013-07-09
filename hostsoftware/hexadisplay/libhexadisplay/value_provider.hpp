#ifndef LIBHEXADISPLAY_VALUE_PROVIDER_HPP
#define LIBHEXADISPLAY_VALUE_PROVIDER_HPP 1

#include <libhexadisplay/common.hpp>
#include <libklio/store.hpp>
#include <libklio/store-factory.hpp>
#include <libklio/sensor.hpp>
#include <libklio/sensor-factory.hpp>
#include <libklio/sensor.hpp>
#include <libklio/time.hpp>
#include <QString>

namespace hexadisplay {

  class ValueProvider {
    public:
      typedef std::tr1::shared_ptr<ValueProvider> Ptr;
      ValueProvider (const std::string& configfile);
      QString get_temperature() {
        return get_last_reading(_tempsensor); }
      QString get_humidity() {
        return get_last_reading(_humiditysensor); }
      QString get_power() {
        return get_last_reading(_powersensor); }
      QString get_pressure() {
        return get_last_reading(_pressuresensor); }
      virtual ~ValueProvider() {};

    private:
      ValueProvider (const ValueProvider& original);
      ValueProvider& operator= (const ValueProvider& rhs);
      QString get_last_reading(klio::Sensor::Ptr sensor);
      klio::Store::Ptr _store;
      klio::Sensor::Ptr _tempsensor;
      klio::Sensor::Ptr _pressuresensor;
      klio::Sensor::Ptr _powersensor;
      klio::Sensor::Ptr _humiditysensor;
  };

}

#endif /* LIBHEXADISPLAY_VALUE_PROVIDER_HPP */

