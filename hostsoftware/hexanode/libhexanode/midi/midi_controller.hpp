#ifndef LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP
#define LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP 1

#include <libhexanode/common.hpp>
#include <RtMidi.h>

namespace hexanode {
  class MidiController {
    public:
      typedef boost::shared_ptr<MidiController> Ptr;
      MidiController ();
      virtual ~MidiController();

      void open();

      uint16_t num_ports();
      void set_device_listener( uint16_t port, RtMidiIn::RtMidiCallback callback);

    private:
      MidiController (const MidiController& original);
      MidiController& operator= (const MidiController& rhs);
      RtMidiIn* _midiin;

  };
};

#endif /* LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP */
