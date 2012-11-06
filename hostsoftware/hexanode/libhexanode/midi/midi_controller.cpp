#include "midi_controller.hpp"
#include <libhexanode/error.hpp>

using namespace hexanode;

MidiController::MidiController() 
  : _t()
  , _terminate_midievent_loop(false)
  , _midiin()
  , _on_midievent()
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

boost::signals2::connection MidiController::do_on_midievent(
    uint16_t port,
    const on_midievent_slot_t& slot)
{
    _midiin->openPort( port );
    // Don't ignore sysex, timing, or active sensing messages.
    _midiin->ignoreTypes( false, false, false );

    //_midiin->setCallback( &internal_callback );
  
  return _on_midievent.connect(slot);
}

void MidiController::run() {
  _t = boost::thread(
        boost::bind(&MidiController::midievent_loop, this)
      );
}

void MidiController::shutdown() {
  _terminate_midievent_loop=true;
  _t.join();
}

void MidiController::midievent_loop() {
  std::vector<unsigned char> message;
  int nBytes;
  //double stamp;
  while (! _terminate_midievent_loop) {
    //stamp = _midiin->getMessage( &message );
    _midiin->getMessage( &message );
    nBytes = message.size();
    if (nBytes > 0) {
      _on_midievent(&message);
      //std::cout << "stamp = " << stamp << std::endl;
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

void MidiController::print_ports() {
  std::string portName;
  for ( unsigned int i=0; i<num_ports(); i++ ) {
    portName = _midiin->getPortName(i);
    std::cout << "  Input Port #" << i+1 << ": " << portName << std::endl;
  }
}
