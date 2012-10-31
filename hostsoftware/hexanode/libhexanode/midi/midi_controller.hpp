#ifndef LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP
#define LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP 1

#include <libhexanode/common.hpp>
#include <boost/signals2.hpp>
#include <boost/thread/thread.hpp>
#include <RtMidi.h>

namespace hexanode {
  class MidiController {
    public:
      typedef boost::shared_ptr<MidiController> Ptr;

      typedef boost::signals2::signal<void 
        (std::vector<unsigned char>*)> on_midievent_t;
      typedef on_midievent_t::slot_type on_midievent_slot_t;
      MidiController ();
      virtual ~MidiController();

      void open();
      boost::signals2::connection do_on_midievent(
          uint16_t port,
          const on_midievent_slot_t& slot);
      void run();
      void shutdown();

      uint16_t num_ports();
      void print_ports();

    private:
      MidiController (const MidiController& original);
      MidiController& operator= (const MidiController& rhs);
      void midievent_loop();

      boost::thread _t;
      bool _terminate_midievent_loop;
      RtMidiIn* _midiin;
      on_midievent_t _on_midievent;

  };
};

#endif /* LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP */
