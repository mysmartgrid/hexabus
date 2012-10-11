#include <libhbc/graph_simplification.hpp>

using namespace hexabus;

command_block_doc GraphSimplification::commandBlockTail(command_block_doc& commands) {
  command_block_doc tail_block = commands;
  tail_block.commands.erase(tail_block.commands.begin());

  return tail_block;
}

void GraphSimplification::addTransition(vertex_id_t from, vertex_id_t to, command_block_doc& commands) {
  // remove old edge
  boost::remove_edge(from, to, *_g);

  // make new vertices and edges
  // add new state vertex
  vertex_id_t new_state_vertex = boost::add_vertex(*_g);
  // TODO (*_g)[new_state_vertex].name = oss.str();
  // TODO (*_g)[new_state_vertex].machine_id = statemachine.id;
  // (*_g)[new_state_vertex].vertex_id = i; TODO there has to be a set of IDs reserved for vertices added through simplification
  (*_g)[new_state_vertex].type = v_state;

  // add edge from old command vertex to new state vertex (into-edge)
  edge_id_t into_edge;
  bool into_edge_ok;
  boost::tie(into_edge, into_edge_ok) = boost::add_edge(from, new_state_vertex, *_g);
  if(into_edge_ok) {
    (*_g)[into_edge].type = e_from_state;
  } else {
    // TODO exeption
  }

  // add new if-vertex
  condition_doc if_true = 1U; // a condition_doc which is of type unsigned int and contains "1" is interpreted as "true".
  vertex_id_t new_if_vertex = boost::add_vertex(*_g);
  (*_g)[new_if_vertex].name = "[s] if (true)";
  // TODO (*_g)[new_if_vertex].machine_id = statemachine.id;
  // TODO (*_g)[new_if_vertex].vertex_id;
  (*_g)[new_if_vertex].type = v_cond;
  (*_g)[new_if_vertex].contents = if_true;

  // add edge to this vertex
  edge_id_t if_edge;
  bool if_edge_ok;
  boost::tie(if_edge, if_edge_ok) = boost::add_edge(new_state_vertex, new_if_vertex, *_g);
  if(if_edge_ok) {
    (*_g)[if_edge].type = e_from_state;
  } else {
    // TODO exception
  }

  // add new command vertex
  vertex_id_t new_command_vertex = boost::add_vertex(*_g);
  // TODO (*_g)[new_command_vertex].name = ...
  // TODO (*_g)[new_command_vertex].machine_id = statemachine.id;
  // TODO (*_g)[new_command_vertex].vertex_id = ...?
  (*_g)[new_command_vertex].type = v_command;
  (*_g)[new_command_vertex].contents = commands;

  // add edge to this vertex
  edge_id_t command_edge;
  bool command_edge_ok;
  boost::tie(command_edge, command_edge_ok) = boost::add_edge(new_if_vertex, new_command_vertex, *_g);
  if(command_edge_ok) {
    (*_g)[command_edge].type = e_if_com;
  } else {
    // TODO exception
  }

  // add edge from new command vertex to old to-state-vertex
  edge_id_t to_edge;
  bool to_edge_ok;
  boost::tie(to_edge, to_edge_ok) = boost::add_edge(new_command_vertex, to, *_g);
  if(to_edge_ok) {
    (*_g)[to_edge].type = e_to_state;
  } else {
    // TODO exception
  }
}

// TODO this is how I think this should work:
// - look for command nodes
// - each command node containing more than one write command gets split up into:
//   - the original command node, with everything but the first command removed
//   - a state
//   - an outgoing transition from this state with if(true)
//   - the "tail" of the command list from the original command node
//   - repeat recursively until tail is empty.
