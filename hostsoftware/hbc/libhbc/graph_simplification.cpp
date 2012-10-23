#include <libhbc/graph_simplification.hpp>

#include <libhbc/hbc_printer.hpp>
#include <libhbc/error.hpp>

using namespace hexabus;

command_block_doc GraphSimplification::commandBlockTail(command_block_doc& commands) {
  command_block_doc tail_block = commands;
  tail_block.commands.erase(tail_block.commands.begin());
  // TODO throw exception if tail is empty (rather: Figure out WHERE this exc should be thrown!)

  return tail_block;
}

command_block_doc GraphSimplification::commandBlockHead(command_block_doc& commands) {
  command_block_doc head_block = commands;
  head_block.commands.erase(head_block.commands.begin() + 1, head_block.commands.end());
  // TODO throw exc if we got an empty list

  return head_block;
}

vertex_id_t GraphSimplification::addTransition(vertex_id_t from, vertex_id_t to, command_block_doc& commands, graph_t_ptr g, unsigned int& max_vertex_id) { // TODO Rename!

  // remove all but first command from from-vertex
  (*g)[from].contents = commandBlockHead(commands);
  // update vertex name
  hbc_printer pr;
  std::ostringstream from_oss;
  pr(boost::get<command_block_doc>((*g)[from].contents), from_oss);
  std::string from_str = from_oss.str();
  replaceNewline(from_str);
  (*g)[from].name = from_str;

  // remove old edge
  boost::remove_edge(from, to, *g);

  // make new vertices and edges
  // add new state vertex
  vertex_id_t new_state_vertex = add_vertex(g, "intermediate\\nstate", (*g)[from].machine_id, ++max_vertex_id, v_state);

  // add edge from old command vertex to new state vertex (into-edge)
  add_edge(g, from, new_state_vertex, e_from_state);

  // add new if-vertex
  condition_doc if_true = 1U; // a condition_doc which is of type unsigned int and contains "1" is interpreted as "true".
  vertex_id_t new_if_vertex = add_vertex(g, "[I] if(true)", (*g)[from].machine_id, ++max_vertex_id, v_cond, if_true);

  // add edge to this vertex
  add_edge(g, new_state_vertex, new_if_vertex, e_from_state);

  // add new command vertex
  vertex_id_t new_command_vertex = add_vertex(g, "",(*g)[from].machine_id, ++max_vertex_id, v_command, commandBlockTail(commands));

  // generate new vertex name
  std::ostringstream newcmd_oss;
  pr(boost::get<command_block_doc>((*g)[new_command_vertex].contents), newcmd_oss);
  std::string newcmd_str = newcmd_oss.str();
  replaceNewline(newcmd_str);
  (*g)[new_command_vertex].name = newcmd_str;

  // add edge to this vertex
  add_edge(g, new_if_vertex, new_command_vertex, e_if_com);

  // add edge from new command vertex to old to-state-vertex
  add_edge(g, new_command_vertex, to, e_to_state);

  return new_command_vertex;
}

void GraphSimplification::expandMultipleWriteNode(vertex_id_t vertex_id, graph_t_ptr g, unsigned int& max_vertex_id) {
  // assumption: vertex_id points to a command block; command_block.commands in the vertex given by vertex_id has more than 1 element (we should throw an exception somewhere around here if this is not the case)

  vertex_t vertex = (*g)[vertex_id];

  if(vertex.type != v_command)
    throw GraphTransformationErrorException("Non-command-vertex assigned for command extension!");

  try {
    command_block_doc cmdblck = boost::get<command_block_doc>(vertex.contents);

    graph_t::adjacency_iterator outIt, outEnd;
    boost::tie(outIt, outEnd) = boost::adjacent_vertices(vertex_id, *g);
    if(distance(outIt, outEnd) != 1)
      throw GraphTransformationErrorException("Multiple outgoint edges on command graph during graph simplification");
    // now outIt points to the state vertex after the command vertex.
    vertex_id_t new_command_vertex = addTransition(vertex_id, *outIt, cmdblck, g, max_vertex_id);

    // if the "tail" still has more than one command, expand it more (recursively)
    try {
      if(boost::get<command_block_doc>((*g)[new_command_vertex].contents).commands.size() > 1) {
        expandMultipleWriteNode(new_command_vertex, g, max_vertex_id);
      }
    } catch(boost::bad_get b) {
      throw GraphTransformationErrorException("Newly created command vertex does not have command block as contents");
    }

  } catch(boost::bad_get b) {
    throw GraphTransformationErrorException("Non-command-vertex assigned for command extension!");
  }

}

void GraphSimplification::expandMultipleWrites() {
  for(std::map<std::string, graph_t_ptr>::iterator it = _in_state_machines.begin(); it != _in_state_machines.end(); it++) { // iterate over list of device->state machine pairs // TODO we can move this loop into the operator() method for readability.
    graph_t::vertex_iterator vertexIt, vertexEnd;

    // iterate over the graph once to find the maximum vertex id (so we can number the vertices we generate from there)
    boost::tie(vertexIt, vertexEnd) = vertices(*(it->second));
    unsigned int max_vertex_id = 0;
    for(; vertexIt != vertexEnd; vertexIt++)
      if((*(it->second))[*vertexIt].vertex_id >= max_vertex_id)
        max_vertex_id = (*(it->second))[*vertexIt].vertex_id;

    // now iterate over the graph to find command vertices with multiple write commands and expand them
    boost::tie(vertexIt, vertexEnd) = vertices(*(it->second));
    for(; vertexIt != vertexEnd; vertexIt++) {
      vertex_t& vertex = (*(it->second))[*vertexIt];

      if(vertex.type == v_command) { // TODO find out how to move the code from here into the loop in deleteOthersWrites.
        try {
          std::vector<command_doc>& cmds = boost::get<command_block_doc>(vertex.contents).commands;

          if(cmds.size() > 1) {
            expandMultipleWriteNode(*vertexIt, it->second, max_vertex_id);
          }

        } catch(boost::bad_get b) { // TODO report line number
          throw GraphTransformationErrorException("Command block vertex does not contain command block (during graph simplification)");
        }
      }
    }
  }
}

void GraphSimplification::deleteOthersWrites() {
  for(std::map<std::string, graph_t_ptr>::iterator it = _in_state_machines.begin(); it != _in_state_machines.end(); it++) { // iterate over list of device->state machine pairs // TODO we can move this loop into the operator() method for readability.
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
            try {
              if(boost::get<std::string>(cmd.write_command.geid.device_alias) != it->first)
                cmds.erase(cmds.begin() + i);
              else
                i++; // here we increment the index: Only if nothing was deleted. Otherwise we have to check the same index again because the elements are all moved one "to the left".
            } catch(boost::bad_get b) {
              throw InvalidPlaceholderException("Placeholder instead of device name found (during graph simplification)."); // TODO can we find out the file name and line number here?
            }
          }
          // update vertex name
          hbc_printer pr;
          std::ostringstream oss;
          pr(boost::get<command_block_doc>(vertex.contents), oss);
          std::string str = oss.str();
          replaceNewline(str);
          vertex.name = str;
        } catch(boost::bad_get b) {
          throw GraphTransformationErrorException("Command block vertex does not contain command block (during graph simplification)");
        }
      }
    }
  }
}

void GraphSimplification::operator()() {
  deleteOthersWrites(); // (1)
  expandMultipleWrites(); // (2)
}


// TODO this is how I think this should work:
// - iterate over the list of graphs <-- TODO
// - look for command nodes
//   - delete all the write commands writing onto OTHER devices (1)
//   - each command node STILL containing more than one write command gets split up into: (2)
//     - the original command node, with everything but the first command removed
//     - a state
//     - an outgoing transition from this state with if(true)
//     - the "tail" of the command list from the original command node
//     - repeat recursively until tail is empty.
