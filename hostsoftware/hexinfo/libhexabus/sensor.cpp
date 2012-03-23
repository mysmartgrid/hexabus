#include "sensor.hpp"

using namespace hexabus;
using namespace boost::posix_time;

const boost::posix_time::ptime Sensor::get_timestamp() const {
  return second_clock::local_time();
}

const std::string Sensor::get_id() const {
  return _id;
}
