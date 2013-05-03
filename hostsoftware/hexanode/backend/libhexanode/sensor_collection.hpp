#ifndef LIBHEXANODE_SENSOR_COLLECTION_HPP
#define LIBHEXANODE_SENSOR_COLLECTION_HPP 1

#include <libhexanode/common.hpp>
#include <libhexanode/sensor.hpp>
#include <map>

/**
 * this scaffold duplicates functionality that is already present
 * in libklio. TODO: Remove and replace using libklio. This would also 
 * simplify the calculation of min/max values.
 */

namespace hexanode {
  class SensorStore {
    public:
      typedef boost::shared_ptr<SensorStore> Ptr;
      typedef std::map<std::string, Sensor::Ptr> sensorstore_t;
      typedef sensorstore_t::const_iterator s_const_iterator_t;
      SensorStore () 
        : _collection() {};
      virtual ~SensorStore() {}; 

      void add_sensor(Sensor::Ptr new_sensor);
      Sensor::Ptr get_by_id(const std::string& sensor_id);

      sensorstore_t::size_type size() { return _collection.size(); }
      s_const_iterator_t begin() { return _collection.begin(); }
      s_const_iterator_t end() { return _collection.end(); }

    private:
      SensorStore (const SensorStore& original);
      SensorStore& operator= (const SensorStore& rhs);
      sensorstore_t _collection;
      
  };
  
};


#endif /* LIBHEXANODE_SENSOR_COLLECTION_HPP */

