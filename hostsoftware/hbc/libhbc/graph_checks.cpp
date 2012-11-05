#include "graph_checks.hpp"

#include <libhbc/error.hpp>

using namespace hexabus;

void GraphChecks::operator()() {
  // TODO
  // simple checks:
  // - Non-reachable states:
  //   - States without inbound edges
  //   - Isolated states (states which cannot be found with BFS from the init node.
  // - Terminal states: States without outbound edges
  // more complex checks:
  // - safety / liveness checks:
  // - code annotation:
  //   - Define a set of states which can be reached only once during execution
  //   - Set of states that are immediately left
  //   - Set of states that, once the machine is in there, can never be left again
}

void GraphChecks::find_unreachable_states() {
  // simple case: A state has no inbound edges.
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for(; vertexIt != vertexEnd; vertexIt++) {
    vertex_id_t vertex_id = *vertexIt;
    vertex_t vertex = (*_g)[vertex_id];
    if(vertex.type == v_state) {
      // check that it's not the init state
      if(vertex.name.length() < 5 || vertex.name.substr(vertex.name.length() - 5) != ".init") {
        graph_t::inv_adjacency_iterator inedgeIt, inedgeEnd;
        boost::tie(inedgeIt, inedgeEnd) = inv_adjacent_vertices(vertex_id, (*_g));
        if (std::distance(inedgeIt, inedgeEnd) == 0) {
          std::cout << "State '(" << vertex.machine_id << "." << vertex.vertex_id << ") " << vertex.name.substr(vertex.name.find("\\n") + 2) // cut off the (n.n) \n which was added for the graphviz output.
            << "' has no inbound edges and is not an initial state." << std::endl;
        }
      }
    }
  }

  // now perform a breadth first search to find states which can't be reached from the initial state.
  // search should begin at "init" state.
  unsigned int init_v_id, init_m_id;
  bool found_init = false;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for(; vertexIt != vertexEnd; vertexIt++) {
    std::string v_name = (*_g)[*vertexIt].name;
    if(v_name.length() > 5) { // make sure it's long enough to contain the substring
      if(v_name.substr(v_name.length() - 5) == ".init") {
        init_v_id = (*_g)[*vertexIt].vertex_id;
        init_m_id = (*_g)[*vertexIt].machine_id;
        found_init = true;
        break;
      }
    }
  }
  if(found_init) {
    std::vector<vertex_id_t> reachable_nodes;
    boost::breadth_first_search(*_g, find_vertex(_g, init_m_id, init_v_id), visitor(bfs_nodelist_maker(&reachable_nodes)));
    std::cout << "Graph checks: Reachable states:" << std::endl;
    BOOST_FOREACH(vertex_id_t s, reachable_nodes) {
      std::cout << "  " << (*_g)[s].name << std::endl;
    }
  } else {
    std::cout << "Graph checks: No init state found." << std::endl; // TODO machine ID
  }
}

