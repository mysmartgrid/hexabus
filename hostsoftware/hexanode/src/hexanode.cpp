#include <iostream>
#include <RtMidi.h>
#include <cstdlib>
#include <libhexanode/midi/midi_controller.hpp>
#include <libhexanode/callbacks/print_callback.hpp>
#include <libhexanode/error.hpp>

void mycallback( 
    double deltatime, 
    std::vector< unsigned char > *message, 
    void *userData )
{
  unsigned int nBytes = message->size();
  for ( unsigned int i=0; i<nBytes; i++ )
    std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
  if ( nBytes > 0 )
    std::cout << "stamp = " << deltatime << std::endl;
}

int main (int argc, char const* argv[]) {
  std::cout << "HexaNode!" << std::endl;

  hexanode::MidiController::Ptr midi_ctrl(
      new hexanode::MidiController());
  hexanode::Callback::Ptr printer(
      new hexanode::PrintCallback());
  try {
    midi_ctrl->open();

    // Check inputs.
    uint16_t nPorts = midi_ctrl->num_ports();
    std::cout << "There are " << nPorts 
      << " MIDI input sources available." << std::endl;
    if (nPorts == 0) {
      std::cout << "Sorry, cannot continue without MIDI input devices." << std::endl;
      exit(-1);
    }
    //
    //  std::string portName;
    //  for ( unsigned int i=0; i<nPorts; i++ ) {
    //    try {
    //      portName = midiin->getPortName(i);
    //    } catch ( RtError &error ) {
    //      error.printMessage();
    //      goto cleanup;
    //    }
    //    std::cout << "  Input Port #" << i+1 << ": " << portName << std::endl;
    //  }
    //

    midi_ctrl->do_on_event(0, boost::bind(
          &hexanode::Callback::on_event,
          printer, _1));
    midi_ctrl->run();
    std::cout << "\nReading MIDI input ... press <enter> to quit.\n";
    char input;
    std::cin.get(input);
  } catch (const hexanode::MidiException& ex) {
    std::cerr << "MIDI error: " << ex.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Unknown error: " << e.what() << std::endl;
  }

  midi_ctrl->shutdown();
  return 0;
}

