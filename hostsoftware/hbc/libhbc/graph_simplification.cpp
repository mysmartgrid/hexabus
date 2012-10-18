#include <libhbc/graph_simplification.hpp>

#include <iostream>
#include <libhbc/hbc_printer.hpp>

using namespace hexabus;

command_block_doc GraphSimplification::commandBlockTail(command_block_doc& commands) {
  command_block_doc tail_block = commands;
  tail_block.commands.erase(tail_block.commands.begin());

  return tail_block;
}

void GraphSimplification::addTransition(vertex_id_t from, vertex_id_t to, command_block_doc& commands) {
// TODO: Pass the graph as parameter, since there are several graphs involved here!
//  // remove old edge
//  boost::remove_edge(from, to, *_g);
//
//  // make new vertices and edges
//  // add new state vertex
//  vertex_id_t new_state_vertex = add_vertex(_g, "TODO" /*TODO*/, 0 /*TODO*/, 0 /*TODO*/, v_state);
//
//  // add edge from old command vertex to new state vertex (into-edge)
//  add_edge(_g, from, new_state_vertex, e_from_state);
//
//  // add new if-vertex
//  condition_doc if_true = 1U; // a condition_doc which is of type unsigned int and contains "1" is interpreted as "true".
//  vertex_id_t new_if_vertex = add_vertex(_g, "[s] if(true)", 0,0/*TODO*/, v_cond, if_true);
//
//  // add edge to this vertex
//  add_edge(_g, new_state_vertex, new_if_vertex, e_from_state);
//
//  // add new command vertex
//  vertex_id_t new_command_vertex = add_vertex(_g, "...",0,0/*TODO*/, v_command, commands);
//
//  // add edge to this vertex
//  add_edge(_g, new_if_vertex, new_command_vertex, e_if_com);
//
//  // add edge from new command vertex to old to-state-vertex
//  add_edge(_g, new_command_vertex, to, e_to_state);
}

void GraphSimplification::deleteOthersWrites() {
  for(std::map<std::string, graph_t_ptr>::iterator it = _in_state_machines.begin(); it != _in_state_machines.end(); it++) { // iterate over list of device->state machine pairs
    graph_t::vertex_iterator vertexIt, vertexEnd;
    boost::tie(vertexIt, vertexEnd) = vertices(*(it->second));
    for(; vertexIt != vertexEnd; vertexIt++) { // iterate over vertices of device's state machine
      vertex_t& vertex = (*(it->second))[*vertexIt];

      if(vertex.type == v_command) {
        try {
          std::vector<command_doc>& cmds = boost::get<command_block_doc>(vertex.contents).commands;
          // delete command nodes for other devices
          for(unsigned int i = 0; i < cmds.size();/* increment only if nothing was deleted */) {
            command_doc cmd = cmds[i];
            if(boost::get<std::string>(cmd.write_command.geid.device_alias) != it->first)
              cmds.erase(cmds.begin() + i);
            else
              i++;
          }
          // update vertex name
          hbc_printer pr;
          std::ostringstream oss;
          pr(boost::get<command_block_doc>(vertex.contents), oss);
          std::string str = oss.str();
          replaceNewline(str);
          vertex.name = str;
        } catch(boost::bad_get b) {
          // TODO
          // -- an exc in the first get is an error during graph transformation
          // -- an exc in the secont get (inner loop) is a programming error OR an error during graph const (placeholder in state machine instance!)
        }
      }
    }
  }
}

void GraphSimplification::operator()() {
  deleteOthersWrites();
}


// TODO this is how I think this should work:
// - iterate over the list of graphs
// - look for command nodes
//   - delete all the write commands writing onto OTHER devices
//   - each command node STILL containing more than one write command gets split up into:
//     - the original command node, with everything but the first command removed
//     - a state
//     - an outgoing transition from this state with if(true)
//     - the "tail" of the command list from the original command node
//     - repeat recursively until tail is empty.
