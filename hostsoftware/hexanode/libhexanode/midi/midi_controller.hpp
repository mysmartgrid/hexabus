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
        (std::vector<unsigned char>*)> on_event_t;
      typedef on_event_t::slot_type on_event_slot_t;
      MidiController ();
      virtual ~MidiController();

      void open();
      boost::signals2::connection do_on_event(
          uint16_t port,
          const on_event_slot_t& slot);
      void run();
      void shutdown();

      uint16_t num_ports();

    private:
      MidiController (const MidiController& original);
      MidiController& operator= (const MidiController& rhs);
      void event_loop();

      boost::thread _t;
      bool _terminate_event_loop;
      RtMidiIn* _midiin;
      on_event_t _on_event;

  };
};

#endif /* LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP */
