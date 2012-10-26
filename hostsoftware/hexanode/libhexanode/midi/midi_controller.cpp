#include "midi_controller.hpp"
#include <libhexanode/error.hpp>

using namespace hexanode;

MidiController::MidiController() 
  : _t()
  , _terminate_event_loop(false)
  , _midiin()
  , _on_event()
{ };

MidiController::~MidiController() {
  if (_midiin != NULL)
    delete _midiin;
}

void MidiController::open() {
  try {
    _midiin = new RtMidiIn();
  } catch (const RtError& error) {
    throw MidiException(error.getMessage());
  }
}

boost::signals2::connection MidiController::do_on_event(
    uint16_t port,
    const on_event_slot_t& slot)
{
    _midiin->openPort( port );
    // Don't ignore sysex, timing, or active sensing messages.
    _midiin->ignoreTypes( false, false, false );

    //_midiin->setCallback( &internal_callback );
  
  return _on_event.connect(slot);
}

void MidiController::run() {
  _t = boost::thread(
        boost::bind(&MidiController::event_loop, this)
      );
}

void MidiController::shutdown() {
  _terminate_event_loop=true;
  _t.join();
}

void MidiController::event_loop() {
  std::vector<unsigned char> message;
  int nBytes;
  double stamp;
  while (! _terminate_event_loop) {
    stamp = _midiin->getMessage( &message );
    nBytes = message.size();
    if (nBytes > 0) {
      _on_event(&message);
      std::cout << "stamp = " << stamp << std::endl;
    }

    // Sleep for 10 milliseconds ... platform-dependent.
    boost::this_thread::sleep(boost::posix_time::milliseconds(20));
  }
}

uint16_t MidiController::num_ports() {
  uint16_t retval=0;
  try {
    retval = _midiin->getPortCount();
  } catch (const RtError& error) {
    throw MidiException(error.getMessage());
  }
  return retval;
}

