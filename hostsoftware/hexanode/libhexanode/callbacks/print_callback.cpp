#include "print_callback.hpp"


using namespace hexanode;


void PrintCallback::on_event(
    std::vector< unsigned char >* message)
{
  unsigned int nBytes = message->size();
  for ( unsigned int i=0; i<nBytes; i++ )
    std::cout << "Byte " << i << " = " << (int)message->at(i) << ", ";
}
