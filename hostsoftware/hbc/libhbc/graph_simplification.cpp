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
  vertex_id_t new_state_vertex = add_vertex(_g, "TODO" /*TODO*/, 0 /*TODO*/, 0 /*TODO*/, v_state);

  // add edge from old command vertex to new state vertex (into-edge)
  add_edge(_g, from, new_state_vertex, e_from_state);

  // add new if-vertex
  condition_doc if_true = 1U; // a condition_doc which is of type unsigned int and contains "1" is interpreted as "true".
  vertex_id_t new_if_vertex = add_vertex(_g, "[s] if(true)", 0,0/*TODO*/, v_cond, if_true);

  // add edge to this vertex
  add_edge(_g, new_state_vertex, new_if_vertex, e_from_state);

  // add new command vertex
  vertex_id_t new_command_vertex = add_vertex(_g, "...",0,0/*TODO*/, v_command, commands);

  // add edge to this vertex
  add_edge(_g, new_if_vertex, new_command_vertex, e_if_com);

  // add edge from new command vertex to old to-state-vertex
  add_edge(_g, new_command_vertex, to, e_to_state);
}

// TODO this is how I think this should work:
// - look for command nodes
// - each command node containing more than one write command gets split up into:
//   - the original command node, with everything but the first command removed
//   - a state
//   - an outgoing transition from this state with if(true)
//   - the "tail" of the command list from the original command node
//   - repeat recursively until tail is empty.
