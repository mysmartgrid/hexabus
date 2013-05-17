#include "sensor_collection.hpp"
#include <libhexanode/error.hpp>


using namespace hexanode;

void SensorStore::add_sensor(Sensor::Ptr new_sensor) {
  // TODO: Prevent adding a sensor several times.
  try {
    Sensor::Ptr found = get_by_id(new_sensor->get_id());
    throw DuplicateException("Sensor already known.");
  } catch (NotFoundException nfe) {
    _collection[new_sensor->get_id()] = new_sensor;
  }
}


Sensor::Ptr SensorStore::get_by_id(const std::string& sensor_id) {
  s_const_iterator_t it=_collection.find(sensor_id);
  if (it != _collection.end()) {
    return (it->second);
  } else {
    throw NotFoundException("Sensor not found.");
  }
}


