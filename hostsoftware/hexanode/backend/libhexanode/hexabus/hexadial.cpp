#include "hexadial.hpp"
#include <algorithm>
#include <utility>
#include <boost/thread/locks.hpp> 

using namespace hexanode;



void HexaDial::on_event(const uint8_t dial_id, const uint8_t dial_position) {
  boost::lock_guard<boost::mutex> guard(_mtx);
  _values[dial_id] = dial_position;
}

void HexaDial::send_values() {
  boost::lock_guard<boost::mutex> guard(_mtx);
  if (! map_equal(_values, _old_values)) {
    for( dial_positions_it_t it = _values.begin(); 
        it != _values.end(); ++it) {
      uint8_t dial_id = it->first;
      uint8_t dial_position = it->second;
      if (_old_values[dial_id] != dial_position) {
        uint8_t epid = 33+dial_id-1;
        if (epid > 40) {
          std::cout << "Endpoint ID is out of range: " 
            << (uint16_t) epid << std::endl;
        } else {
          //std::cout << "Sending out dial " << (uint16_t) dial_id 
          //  << " value: " << (uint16_t) dial_position << std::endl;
          _socket.send(hexabus::InfoPacket<uint8_t>(epid, dial_position));
        }
      }
    }
    _old_values.clear();
    _old_values.insert(_values.begin(), _values.end());
  }
}
