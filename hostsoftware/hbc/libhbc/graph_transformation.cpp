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

  // Iterate over all vertices, get state machine IDs
  // For each state machine ID:
  //   Construct set of states for this ID (vector or something  -- ooh! ooh! Map<st.m.id,vector<state>>)
  //   Do magic with this set.
  //     Magic: Look for all device names in the machine.
  //     FOR each device name, #slice# it -- slices are magic items we need.

  // Sliced machines which fall out of the magic cauldron:
  //   They have to be stored in some new set.
  //   I'd like to have a map<device name,machine>, but at this point, we can have multiple machines with the same device name
  //      -> map<pair<device,stm.id>,machine> perhaps??? --- NO! Multimap!!
  //   Now, for the second bit of magic: IF there is a device name with multiple state machines attached to it
  //      Parallel compose the state machines.

  std::map<unsigned int, std::vector<vertex_t> > machines_per_stmid; // list of nodes for each state machine ID

  // iterate over all vertices in graph
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*in_g));
  for(; vertexIt != vertexEnd; vertexIt++) {
    vertex_id_t vertexID = *vertexIt;
    vertex_t& vertex = (*in_g)[vertexID];
    // look for machine ID in map
    std::map<unsigned int, std::vector<vertex_t> >::iterator node_it;
    if((node_it = machines_per_stmid.find(vertex.machine_id)) != machines_per_stmid.end())
      // if there, append found vertex to vector in map
      node_it->second.push_back(vertex);
    else {
      std::vector<vertex_t> vertex_vect;
      vertex_vect.push_back(vertex);
      // if not there, make new entry in map
      machines_per_stmid.insert(std::pair<unsigned int, std::vector<vertex_t> >(vertex.machine_id, vertex_vect));
    }
  }
  // okay. now we have a map mapping from each state machine ID to the vertices belonging to this state machine.


}
