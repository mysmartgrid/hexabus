#include "button_callback.hpp"
#include <libhexanode/midi/midi_codes.hpp>

using namespace hexanode;

ButtonCallback::ButtonCallback () 
  : Callback() 
  , _on_buttonevent()
{};

void ButtonCallback::on_event(
    std::vector< unsigned char >* message)
{
  unsigned int nBytes = message->size();
  //for ( unsigned int i=0; i<nBytes; i++ )
  //  std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
  if (nBytes == 3) {
    // This is a regular MIDI event.
    // First byte: status byte. We ignore the release event.
    //if (message->at(0) == PAD_RELEASED) {
    //  std::cout << "Released ";
    //}
    if (message->at(0) == PAD_PRESSED) {
      // second byte: note number
      uint8_t pad_id = message->at(1);
      _on_buttonevent(pad_id);
    } else if (message->at(0) == KNOB_TURNED) {
      uint8_t knob_id = message->at(1);
      // The MIDI standard says that values are from 0..127. Let's 
      // scale to full uint8_t range.
      uint8_t knob_value = message->at(2) * 2;
      _on_dialevent(knob_id, knob_value);
    }
  } else {
    std::cout << "received event of unknown type - ignoring." << std::endl;
  }
}

boost::signals2::connection ButtonCallback::do_on_buttonevent(
    const on_buttonevent_slot_t& slot) 
{
  return _on_buttonevent.connect(slot);
}

boost::signals2::connection ButtonCallback::do_on_dialevent(
    const on_dialevent_slot_t& slot) 
{
  return _on_dialevent.connect(slot);
}
