#include "graph_transformation.hpp"

#include <libhbc/error.hpp>

using namespace hexabus;

bool GraphTransformation::contains(std::vector<std::string> v, std::string s) {
  BOOST_FOREACH(std::string str, v) {
    if(str == s)
      return true;
  }
  return false;
}


std::vector<vertex_t> GraphTransformation::slice_for_device(std::string dev_name, std::vector<vertex_t> vertices) {
  // TODO
  // * Make list of all (node containing) write commands for this machine
  // * for each write command, slice (backwards reachability analysis)
  // * take union set of all slices

  // find all nodes containing write commands for the given device
  std::vector<vertex_t> command_nodes_for_device;
  BOOST_FOREACH(vertex_t vert, vertices) {
    
  }
}

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
  //   Construct set of states for this ID (vector or something  -- ooh! ooh! Map<st.m.id,vector<state>>) (1)
  //   Do magic with this set. (2)
  //     Magic: Look for all device names in the machine. (5)
  //     FOR each device name,
  //       #slice# it -- slices are magic items we need.

  // Sliced machines which fall out of the magic cauldron: (3)
  //   They have to be stored in some new set.
  //   I'd like to have a map<device name,machine>, but at this point, we can have multiple machines with the same device name
  //      -> map<pair<device,stm.id>,machine> perhaps??? --- NO! Multimap!! (6)
  //   Now, for the second bit of magic: IF there is a device name with multiple state machines attached to it
  //      Parallel compose the state machines.

  typedef std::map<unsigned int, std::vector<vertex_t> > machine_map;
  machine_map machines_per_stmid; // list of nodes for each state machine ID

  // iterate over all vertices in graph
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*in_g));
  for(; vertexIt != vertexEnd; vertexIt++) {
    vertex_id_t vertexID = *vertexIt;
    vertex_t& vertex = (*in_g)[vertexID];
    // look for machine ID in map
    machine_map::iterator node_it;
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
  // okay. now we have a map mapping from each state machine ID to the vertices belonging to this state machine. (1)

  typedef std::multimap<std::string, std::vector<vertex_t> > device_map;
  device_map machines_per_devname; // list of state machine nodes per device name (3)

  // now go through all the state machines
  for(machine_map::iterator stm_it = machines_per_stmid.begin(); stm_it != machines_per_stmid.end(); stm_it++) {
    std::vector<std::string> device_names;
    BOOST_FOREACH(vertex_t vert, stm_it->second) {
      if(vert.type == v_command) {
        try {
          command_block_doc cmdblck = boost::get<command_block_doc>(vert.contents);
          BOOST_FOREACH(command_doc cmd, cmdblck.commands) {
            try {
              std::string dev_name = boost::get<std::string>(cmd.write_command.geid.device_alias);
              if(!contains(device_names, dev_name))
                device_names.push_back(dev_name);
            } catch(boost::bad_get b) {
              std::ostringstream oss;
              oss << "[" << cmd.write_command.geid.lineno << "] Placeholder in state machine instance. Only literal device names are allowed here." << std::endl;
              throw PlaceholderInInstanceException(oss.str());
            }
          }

        } catch(boost::bad_get b) { // This exception should only occur if something went wrong during graph construction
          std::ostringstream oss;
          oss << "Bad get while trying to get command block out of command vertex!" << std::endl;
          throw GraphTransformationErrorException(oss.str());
        }
      }
    }
    // now device_names contains each device alias used in a write command in the state machine. once. (5)

    // now, slice the machines for each device name.
    BOOST_FOREACH(std::string dev_name, device_names) {
      machines_per_devname.insert(std::pair<std::string, std::vector<vertex_t> >(dev_name, slice_for_device(dev_name, stm_it->second)));
    }
  }

  // now we have our multimap, mapping from each device ID to a one or more state machine slices
  // (machines_per_devname) (6)

  // all that's left TODO: Parallel composition of state machines.
  // (and optimization)
  // (and simplification for compatibility with hexabus assembler)

}
