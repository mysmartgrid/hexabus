#include "midi_controller.hpp"
#include <libhexanode/error.hpp>

using namespace hexanode;

MidiController::MidiController() 
  : _midiin()
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

uint16_t MidiController::num_ports() {
  uint16_t retval=0;
  try {
    retval = _midiin->getPortCount();
  } catch (const RtError& error) {
    throw MidiException(error.getMessage());
  }
  return retval;
}

void MidiController::set_device_listener(
    uint16_t port,
    RtMidiIn::RtMidiCallback callback)
{
  try {
    _midiin->openPort( port );
    _midiin->setCallback( callback );
    // Don't ignore sysex, timing, or active sensing messages.
    _midiin->ignoreTypes( false, false, false );
  } catch (const RtError& error) {
    throw MidiException(error.getMessage());
  }
}

