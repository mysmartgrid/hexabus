#include "switch_device.hpp"
#include <QSettings>
#include <QString>


using namespace hexadisplay;


SwitchDevice::SwitchDevice(
    const std::string& interface, 
    const std::string& ip
  ) : _network(new hexabus::NetworkAccess(interface))
  , _packetm(new hexabus::Packet())
  , _ip(ip)
{ }

SwitchDevice::SwitchDevice(
    const std::string& configfile
  ) : _packetm(new hexabus::Packet())
{ 
  QSettings settings(QString(configfile.c_str()), QSettings::IniFormat);
  std::string interface(
      settings.value( "actors/interface", "").toString()
      .toUtf8().constData()); 
  _ip=settings.value( "actors/plug", "").toString()
      .toUtf8().constData(); 
  std::cout << "Using interface " << interface
    << " to reach " << _ip << std::endl;
  //_network=new hexabus::NetworkAccess();
  _network=new hexabus::NetworkAccess(interface);
}


SwitchDevice::~SwitchDevice() {
  delete _network;
}


void SwitchDevice::on() {
  try {
  hxb_packet_int8 packet = _packetm->write8(1, HXB_DTYPE_BOOL, HXB_TRUE, false);
  _network->sendPacket(_ip, HXB_PORT, (char*)&packet, sizeof(packet));
  } catch (std::exception& e) {
    std::cout << "Exception while sending on packet: " << e.what() << std::endl;
  }
}

void SwitchDevice::off() {
  try{
    hxb_packet_int8 packet = _packetm->write8(1, HXB_DTYPE_BOOL, HXB_FALSE, false);
    _network->sendPacket(_ip, HXB_PORT, (char*)&packet, sizeof(packet));
  } catch (std::exception& e) {
    std::cout << "Exception while sending off packet: " << e.what() << std::endl;
  }
}
