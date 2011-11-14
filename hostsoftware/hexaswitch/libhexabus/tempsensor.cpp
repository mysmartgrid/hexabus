#include "tempsensor.hpp"
#include <sstream>

namespace bt = boost::posix_time;
using namespace hexabus;

const std::string TempSensor::str() const {
  std::ostringstream oss;
  oss << "Temperature sensor " << _id << ", " << _values.size() << " values stored.";
  return oss.str();
}


void TempSensor::add_value(float value) {
  LOG("Adding value " << value << " to sensor " << _id);
  _values.insert( std::pair<bt::ptime, float>(get_timestamp(), value) );
}

void TempSensor::save_values(std::ostream& os) {
  std::map<boost::posix_time::ptime, float>::iterator it;
  for ( it=_values.begin() ; it != _values.end(); it++ ) {
    bt::ptime time = (*it).first;
    float value=(*it).second;
    os << time << "\t" << value << std::endl;
  }
}
