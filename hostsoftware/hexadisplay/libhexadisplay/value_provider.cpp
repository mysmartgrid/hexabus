#include "value_provider.hpp"
#include <QSettings>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem; 
using namespace hexadisplay;

ValueProvider::ValueProvider(const std::string& configfile) 
{
  /**
   * Read the config file.
   */
  std::cout << "Using config file " << configfile << std::endl;
  QSettings settings(QString(configfile.c_str()), QSettings::IniFormat);
  std::string db_path(
      settings.value( "database/path", "").toString()
      .toUtf8().constData()); 
  bfs::path _db(db_path);
  std::string tempid(
      settings.value("sensors/temperature", "foo").toString()
      .toUtf8().constData()); 
  std::string pressureid(
      settings.value("sensors/pressure", "foo").toString()
      .toUtf8().constData()); 
  std::string humidityid(
      settings.value("sensors/humidity", "foo").toString()
      .toUtf8().constData()); 
  std::string powerid(
      settings.value("sensors/power", "foo").toString()
      .toUtf8().constData()); 
  /**
   * Open the klio store.
   */
  klio::StoreFactory::Ptr factory(new klio::StoreFactory()); 
  klio::SensorFactory::Ptr sensor_factory(new klio::SensorFactory());
  _store=factory->openStore(klio::SQLITE3, _db);
  std::cout << "opened store: " << _store->str() << std::endl;
  /**
   * Now, select the right sensors from the database.
   */
  std::vector<klio::Sensor::uuid_t> uuids = _store->getSensorUUIDs();
  std::cout << "Found " << uuids.size() 
    << " sensor(s) in the database." << std::endl;
  std::vector<klio::Sensor::uuid_t>::iterator it;
  for(  it = uuids.begin(); it < uuids.end(); it++) {
    klio::Sensor::Ptr loadedSensor(_store->getSensor(*it));
    std::cout << " * " << loadedSensor->str() << std::endl;
    if (boost::iequals(loadedSensor->name(), tempid)) 
      _tempsensor = loadedSensor;
    else if (boost::iequals(loadedSensor->name(), pressureid)) 
      _pressuresensor = loadedSensor;
    else if (boost::iequals(loadedSensor->name(), powerid)) 
      _powersensor = loadedSensor;
    else if (boost::iequals(loadedSensor->name(), humidityid)) 
      _humiditysensor = loadedSensor;
  }
}

QString ValueProvider::get_last_reading(klio::Sensor::Ptr sensor) {
  klio::reading_t reading = _store->get_last_reading(sensor);
  klio::timestamp_t ts1=reading.first;
  double val1=reading.second;
  std::cout << ts1 << "\t" << val1 << std::endl;
  return QString("%1").arg(val1,0,'f',2);
}
