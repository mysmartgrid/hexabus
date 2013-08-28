#include "hexapush.hpp"

using namespace hexanode;

HexaPush::HexaPush (hexabus::Socket& socket)
  : _socket(socket)
{
}

void HexaPush::on_event(uint8_t pressed_key) {
  _socket.send(hexabus::InfoPacket<uint8_t>(25, pressed_key));
}
