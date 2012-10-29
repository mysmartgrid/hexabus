#include "button_callback.hpp"
#include <libhexanode/midi/midi_codes.hpp>
#include <libhexanode/event.hpp>
#include <libhexanode/key_pressed_event.hpp>

using namespace hexanode;


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
      hexanode::Event::Ptr evt(
          new hexanode::KeyPressedEvent(pad_id));
      std::cout << evt->str() << std::endl;
    }
  } else {
    std::cout << "received event of unknown type - ignoring." << std::endl;
  }
}
