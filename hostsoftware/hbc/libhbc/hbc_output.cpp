#include hbc_output.hpp

using namespace hexabus;

HBCOutput::operator()(std::ostream& ostr) {
  // TODO
  // - find highest state machine index
  //   - this will work differently later, when the machines are partitioned, but for now we assume that each state machine only controls one device
  // - go through all the state machines and translate them
  //   - states to states
  //   - if-conditions to conditions
  //     - this also needs more thinking!! (*)
  //   - write actions to actions
  //     - more thinking: What happens on multiple writes? (*)
  //
  // *) These should be done on graph level, along with the slicing/partitoning.


}
