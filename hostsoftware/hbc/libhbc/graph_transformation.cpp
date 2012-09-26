#include "graph_transformation.hpp"

using namespace hexabus;

void GraphTransformation::operator()(graph_t_ptr in_g) {
  // TODO:                                                | Data structures needed:
  // * iterate over (list of) state machines              |
  // * for each state machine:                            |
  //   * make list of devices                             |
  //   * slice each state machine for each device         | * list of state machine slices for devices
  // * if a device appears in multiple state machines:    |   (at this point one dev. can have several state machines)
  //   -> parallel compose state machine slices           | * now, each dev. only has one st.m. (store as map?)
  // * simplify slices for Hexabus Assembler export       | * use the same map data structure for this
  //
  // different version:
  // we have a table of devices. we could use this to get a list of device names / ip addresses.
  // but: a lot of devices which are write only are in this table. we should use it only to find out
  // whether a device exists, and has the right access levels (write (or something like localwrite)
  // for EPs which are written, broadcast for EPs which conditions depend upon. BUT we should check
  // this in the assembler export stage rather than here.
}
