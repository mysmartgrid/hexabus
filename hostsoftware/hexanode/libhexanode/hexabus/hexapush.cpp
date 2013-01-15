#include "hexapush.hpp"

using namespace hexanode;

HexaPush::HexaPush ()
  : _network(new hexabus::NetworkAccess(hexabus::NetworkAccess::Reliable))
  , _packetm(new hexabus::Packet())
{
}

HexaPush::HexaPush(const std::string& interface) 
  : _network(new hexabus::NetworkAccess(interface, hexabus::NetworkAccess::Reliable))
  , _packetm(new hexabus::Packet())
{
  std::cout << "Using interface " << interface << std::endl;
}

void HexaPush::on_event(uint8_t pressed_key) {
  uint32_t eid = 25;
  unsigned int dtype = HXB_DTYPE_UINT8;
  struct hxb_packet_int8 packet8;
  packet8 = _packetm->write8(eid, dtype, pressed_key, true);
  _network->sendPacket(
      HXB_GROUP, 
      HXB_PORT, 
      (char*)&packet8, 
      sizeof(packet8));
}
