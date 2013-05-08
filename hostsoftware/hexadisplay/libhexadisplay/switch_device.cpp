#include "switch_device.hpp"
#include <QSettings>
#include <QString>


using namespace hexadisplay;


SwitchDevice::SwitchDevice(
    const std::string& interface, 
    const std::string& ip
  ) : _io()
  , _socket(_io, interface)
  , _ip(ip)
  , _switch_state(false)
{ 
  off();
}

std::string SwitchDevice::configIP(const std::string& configfile)
{
  QSettings settings(QString(configfile.c_str()), QSettings::IniFormat);
  return settings.value( "actors/plug", "").toString().toUtf8().constData(); 
}

std::string SwitchDevice::configInterface(const std::string& configfile)
{
  QSettings settings(QString(configfile.c_str()), QSettings::IniFormat);
  return settings.value( "actors/interface", "").toString().toUtf8().constData(); 
}

SwitchDevice::SwitchDevice(
    const std::string& configfile
  ) : _io()
  , _socket(_io, configInterface(configfile))
  , _ip(configIP(configfile))
  , _switch_state(false)
{ 
  std::cout << "Using interface " << configIP(configfile)
    << " to reach " << _ip << std::endl;
  //_network=new hexabus::NetworkAccess();
  off();
}


SwitchDevice::~SwitchDevice() {
}

void SwitchDevice::on() {
  try {
    _socket.send(hexabus::WritePacket<bool>(1, true), boost::asio::ip::address::from_string(_ip).to_v6());
    _switch_state=true;
  } catch (std::exception& e) {
    std::cout << "Exception while sending on packet: " << e.what() << std::endl;
  }
}

void SwitchDevice::off() {
  try{
    _socket.send(hexabus::WritePacket<bool>(1, false), boost::asio::ip::address::from_string(_ip).to_v6());
    _switch_state=false;
  } catch (std::exception& e) {
    std::cout << "Exception while sending off packet: " << e.what() << std::endl;
  }
}


void SwitchDevice::toggle() {
  if (! _switch_state) {
    on();
  } else {
    off();
  }
}
