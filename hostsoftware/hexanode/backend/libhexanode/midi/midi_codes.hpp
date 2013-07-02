#ifndef LIBHEXANODE_MIDI_MIDI_CODES_HPP
#define LIBHEXANODE_MIDI_MIDI_CODES_HPP 1

namespace hexanode {
  // list of midi status codes.
  // see http://www.midimountain.com/midi/midi_status.htm
  enum midi_status_bytes {
    PAD_RELEASED = 128, // ch1 note off
    PAD_PRESSED = 144, // ch1 note on
    KNOB_TURNED = 176 // ch1 control/mode change
  };
};


#endif /* LIBHEXANODE_MIDI_MIDI_CODES_HPP */

