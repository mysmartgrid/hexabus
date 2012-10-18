#include <iostream>
#include <RtMidi.h>
#include <cstdlib>


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
  RtMidiIn  *midiin = 0;

  // RtMidiIn constructor
  try {
    midiin = new RtMidiIn();
  } catch ( RtError &error ) {
    error.printMessage();
    exit( EXIT_FAILURE );
  }

  // Check inputs.
  unsigned int nPorts = midiin->getPortCount();
  std::cout << "There are " << nPorts 
    << " MIDI input sources available." << std::endl;
  std::string portName;
  for ( unsigned int i=0; i<nPorts; i++ ) {
    try {
      portName = midiin->getPortName(i);
    } catch ( RtError &error ) {
      error.printMessage();
      goto cleanup;
    }
    std::cout << "  Input Port #" << i+1 << ": " << portName << std::endl;
  }

  midiin->openPort( 0 );
  // Set our callback function.  This should be done immediately after
  // opening the port to avoid having incoming messages written to the
  // queue.
  midiin->setCallback( &mycallback );

  // Don't ignore sysex, timing, or active sensing messages.
  midiin->ignoreTypes( false, false, false );

  std::cout << "\nReading MIDI input ... press <enter> to quit.\n";
  char input;
  std::cin.get(input);

cleanup:
  delete midiin;
  return 0;
}

