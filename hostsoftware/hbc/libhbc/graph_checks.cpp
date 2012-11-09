#include "graph_checks.hpp"

#include <libhbc/error.hpp>

using namespace hexabus;

void GraphChecks::operator()() {
  // TODO
  // simple checks:
  // - Non-reachable states:
  //   - (done) States without inbound edges
  //   - (done) Isolated states (states which cannot be found with BFS from the init node.
  // - (done) Terminal states: States without outbound edges
  // more complex checks:
  // - safety / liveness checks:
  // - user input:
  //   - Define a (TODO set of) state(s) which can always be reached
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

void GraphChecks::reachable_from_anywhere(std::string state_name, std::string machine_name) {
  std::cout << "Graph checks:" << std::endl;
  // iterate over all vertices
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices(*_g);
  for(; vertexIt != vertexEnd; vertexIt++) {
    vertex_t vertex = (*_g)[*vertexIt];
    // find out whether it's a state vertex
    if(vertex.type == v_state) {
      // find out whether it belongs to our machine
      std::string v_machine_name = vertex.name.substr(vertex.name.find_last_of(' ') + 1);
      v_machine_name = v_machine_name.substr(2, v_machine_name.find_first_of('.') - 2); // name starts at 2, because it is lead by "\n"
      if(v_machine_name == machine_name) {
        // construct list of nodes reachable from there
        std::vector<vertex_id_t> reachable_vertices;
        boost::breadth_first_search(*_g, *vertexIt, visitor(bfs_nodelist_maker(&reachable_vertices)));

        // find out whether the node we are looking for is in that list
        bool reachable_from_there = false;
        BOOST_FOREACH(vertex_id_t reachable_vertex_id, reachable_vertices) {
          vertex_t reachable_vertex = (*_g)[reachable_vertex_id];
          if(reachable_vertex.type == v_state) {
            if(reachable_vertex.name.substr(reachable_vertex.name.find_last_of('.') + 1) == state_name) {
              reachable_from_there = true;
              break;
            }
          }
        }

        // output the results
        if(!reachable_from_there) {
          std::cout << "  state " << machine_name << "." << state_name << " is not reachable from state " << vertex.name.substr(vertex.name.find("\\n") + 2) << "." << std::endl;
        }
      }
    }
  }
}

void GraphChecks::never_reachable(std::string name, std::string machine_name) {
  // TODO Concept:
  // - Calculate shortest paths from INIT node
  // - See if the never-to-be-reached node (probably the one generating an output we don't want??) is in the tree
  // - If so, OH NOES!
  // - Go back to INIT node, output path

  // find init state and target state
  bool target_found = false;
  vertex_id_t init_state, target_vertex;
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices(*_g);
  for(; vertexIt != vertexEnd; vertexIt++) {
    if((*_g)[*vertexIt].type == v_state) {
      // find out machine name; only consider vertives from the machine we are looking at.
      std::string v_machine_name = (*_g)[*vertexIt].name.substr((*_g)[*vertexIt].name.find_last_of(' ') + 1);
      v_machine_name = v_machine_name.substr(2, v_machine_name.find_first_of('.') - 2); // name starts at 2, because it is lead by "\n"
      if(v_machine_name == machine_name) {
        std::string v_name = (*_g)[*vertexIt].name;
        if(v_name.length() > 5) {
          if(v_name.substr(v_name.length() - 5) == ".init") {
            init_state = *vertexIt;
          }
        }
        if(v_name.length() > name.length() + 1) {
          if(v_name.substr(v_name.find_last_of(".")) == ("." + name)) {
            target_vertex = *vertexIt;
            target_found = true;
          }
        }
      }
    }
  }

  if(target_found) {
    // now we have init and target, let's find a path between them.
    // construct predecessor map
    std::map<vertex_id_t, vertex_id_t> predecessor_map;
    // now the path is found by taking the target state, and following the predecessors backwards until we reach init.
    boost::breadth_first_search(*_g, init_state, visitor(bfs_predecessor_map_maker(&predecessor_map)));

    // take the target vertex, and follow the predecessor map until init is reached.
    std::cout << "Graph checks: Path to 'never'-vertex found!" << std::endl;
    std::map<vertex_id_t, vertex_id_t>::iterator predecessor_it;
    while((predecessor_it = predecessor_map.find(target_vertex)) != predecessor_map.end()) {
      std::cout << (*_g)[target_vertex].name << std::endl;
      target_vertex = predecessor_it->second;
    }
  } else {
    std::cout << "Graph checks: Target state " << name << " for 'never'-check of machine " << machine_name << " not found." << std::endl;
  }
}

