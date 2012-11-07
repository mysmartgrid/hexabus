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

void GraphChecks::find_states_without_inbound() {
  std::cout << "Graph checks: States without inbound edges that are not init states:" << std::endl;
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
          std::cout << "  (" << vertex.machine_id << "." << vertex.vertex_id << ") " << vertex.name.substr(vertex.name.find("\\n") + 2) // cut off the (n.n) \n which was added for the graphviz output.
            << std::endl;
        }
      }
    }
  }
}

void GraphChecks::find_states_without_outgoing() {
  std::cout << "Graph checks: States without outgoing edges:" << std::endl;
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for(; vertexIt != vertexEnd; vertexIt++) {
    vertex_id_t vertex_id = *vertexIt;
    vertex_t vertex = (*_g)[vertex_id];
    if(vertex.type == v_state) {
      // check that it's not the init state
      graph_t::adjacency_iterator outedgeIt, outedgeEnd;
      boost::tie(outedgeIt, outedgeEnd) = adjacent_vertices(vertex_id, (*_g));
      if (std::distance(outedgeIt, outedgeEnd) == 0) {
        std::cout << "  (" << vertex.machine_id << "." << vertex.vertex_id << ") " << vertex.name.substr(vertex.name.find("\\n") + 2) // cut off the (n.n) \n which was added for the graphviz output.
          << std::endl;
      }
    }
  }
}

void GraphChecks::find_unreachable_states() {
  // now perform a breadth first search to find states which can't be reached from the initial state.
  // search should begin at each "init" state.
  // Part 1: Make a list of "init" states, and a list of all states (we need that later to see which ones are not reachable)
  std::vector<vertex_id_t> init_states;
  std::set<vertex_id_t> all_states;
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for(; vertexIt != vertexEnd; vertexIt++) {
    if((*_g)[*vertexIt].type == v_state) {
      std::string v_name = (*_g)[*vertexIt].name;
      if(v_name.length() > 5) { // make sure it's long enough to contain the substring, so we don't get an underflow when we do length-5
        if(v_name.substr(v_name.length() - 5) == ".init") {
          init_states.push_back(*vertexIt);
        }
      }
      all_states.insert(*vertexIt);
    }
  }

  // Part 2: Start BFS from each of these init states, making a list of reachable states
  std::vector<vertex_id_t> reachable_nodes;
  BOOST_FOREACH(vertex_id_t init_vertex_id, init_states) {
    boost::breadth_first_search(*_g, init_vertex_id, visitor(bfs_nodelist_maker(&reachable_nodes)));
  }

  // Part 3: Make a list of unreachable states (ones in the graph, but not in the "reachable states" list.)
  std::set<vertex_id_t>& unreachable_states = all_states; // just so the name is semantically OK
  BOOST_FOREACH(vertex_id_t reachable_node, reachable_nodes) {
    unreachable_states.erase(reachable_node); // does nothing (returns 0) if there is none found (i.e. it wasn't a state node)
  }

  std::cout << "Graph checks: Unreachable states:" << std::endl;
  BOOST_FOREACH(vertex_id_t s, unreachable_states) {
    vertex_t vertex = (*_g)[s];
    std::cout << "  (" << vertex.machine_id << "." << vertex.vertex_id << ") " << (*_g)[s].name.substr((*_g)[s].name.find("\\n") + 2)<< std::endl;
  }
}

// TODO This needs to check only a single state machine, not the whole graph!
void GraphChecks::reachable_from_anywhere(std::string name) {
  // iterate over all vertices
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices(*_g);
  for(; vertexIt != vertexEnd; vertexIt++) {
    vertex_t vertex = (*_g)[*vertexIt];
    // find out whether it's a state vertex
    if(vertex.type == v_state) {
    // construct list of nodes reachable from there
      std::vector<vertex_id_t> reachable_vertices;
      boost::breadth_first_search(*_g, *vertexIt, visitor(bfs_nodelist_maker(&reachable_vertices)));

      // find out whether the node we are looking for is in that list
      bool reachable_from_there = false;
      BOOST_FOREACH(vertex_id_t reachable_vertex_id, reachable_vertices) {
        vertex_t reachable_vertex = (*_g)[reachable_vertex_id];
        if(reachable_vertex.type == v_state) {
          if(reachable_vertex.name.substr(reachable_vertex.name.find_last_of('.') + 1) == name) {
            reachable_from_there = true;
            break;
          }
        }
      }

      // output the results
      std::cout << "Graph checks:" << std::endl;
      if(!reachable_from_there) {
        std::cout << "  state " << name << " is not reachable from state " << vertex.name << "." << std::endl;
      }
    }
  }
}

