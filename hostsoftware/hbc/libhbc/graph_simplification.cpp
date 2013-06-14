#include <libhbc/graph_simplification.hpp>

#include <libhbc/hbc_printer.hpp>
#include <libhbc/error.hpp>

using namespace hexabus;

command_block_doc GraphSimplification::commandBlockTail(command_block_doc& commands) {
  command_block_doc tail_block = commands;
  if(tail_block.commands.size() < 2) {
    throw GraphSimplificationException("Empty command block tail.");
  }
  tail_block.commands.erase(tail_block.commands.begin());

  return tail_block;
}

command_block_doc GraphSimplification::commandBlockHead(command_block_doc& commands) {
  command_block_doc head_block = commands;
  if(head_block.commands.size() < 1) {
    throw GraphSimplificationException("Empty command block head.");
  }
  head_block.commands.erase(head_block.commands.begin() + 1, head_block.commands.end());

  return head_block;
}

vertex_id_t GraphSimplification::splitTransition(vertex_id_t from, vertex_id_t to, command_block_doc& commands, graph_t_ptr g, unsigned int& max_vertex_id) {
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
      throw GraphTransformationErrorException("Multiple outgoing edges on command graph during graph simplification");
    // now outIt points to the state vertex after the command vertex.
    vertex_id_t new_command_vertex = splitTransition(vertex_id, *outIt, cmdblck, g, max_vertex_id);

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

void GraphSimplification::expandMultipleWrites(graph_t_ptr g) {
  graph_t::vertex_iterator vertexIt, vertexEnd;

  // iterate over the graph once to find the maximum vertex id (so we can number the vertices we generate from there)
  boost::tie(vertexIt, vertexEnd) = vertices(*g);
  unsigned int max_vertex_id = 0;
  for(; vertexIt != vertexEnd; vertexIt++)
    if((*g)[*vertexIt].vertex_id >= max_vertex_id)
      max_vertex_id = (*g)[*vertexIt].vertex_id;

  // now iterate over the graph to find command vertices with multiple write commands and expand them
  vertexIt = vertices(*g).first;
  for(size_t i = 0; vertexIt != vertices(*g).second; vertexIt = vertices(*g).first + ++i) {
    vertex_t& vertex = (*g)[*vertexIt];

    if(vertex.type == v_command) {
      try {
        std::vector<command_doc>& cmds = boost::get<command_block_doc>(vertex.contents).commands;

        if(cmds.size() > 1) {
          expandMultipleWriteNode(*vertexIt, g, max_vertex_id);
        }

      } catch(boost::bad_get b) {
        throw GraphTransformationErrorException("Command block vertex does not contain command block (during graph simplification)");
      }
    }
  }
}

void GraphSimplification::expandComplexConditions(graph_t_ptr g) {
  // iterate over the graph once to find the maximum vertex id (so we can number the vertices we generate from there) -- TODO we could move this into the operator()-method and keep max_vertices as a class variable
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices(*g);
  unsigned int max_vertex_id = 0;
  for(; vertexIt != vertexEnd; vertexIt++)
    if((*g)[*vertexIt].vertex_id >= max_vertex_id)
      max_vertex_id = (*g)[*vertexIt].vertex_id;

  vertexIt = vertices(*g).first;
  for(size_t i = 0; vertexIt != vertices(*g).second; vertexIt = vertices(*g).first + ++i) {
    vertex_t& vertex = (*g)[*vertexIt];
    // find condition vertices
    if(vertex.type == v_cond) {
      try {
        condition_doc& cond = boost::get<condition_doc>(vertex.contents);
        if(cond.which() == 4) { // if it's a compound condition
          // find out whether it's an AND or an OR condition
          compound_condition_doc comp_cond = boost::get<compound_condition_doc>(cond);
          if(comp_cond.bool_op == AND) {
            // The semantics of AND conditions are still not clear.
            throw GraphSimplificationException("AND operator (&&) not supported yet.");
          } else { // OR
            // Make two conditions; duplicate command vertex
            // what do we have, what do we need?
            // Have: vertex; *vertexId -- the vertex we are at.
            // Need: from-state, command, to-state.

            // find command vertex
            graph_t::adjacency_iterator adjIt, adjEnd;
            boost::tie(adjIt, adjEnd) = adjacent_vertices(*vertexIt, *g);
            vertex_id_t command_vertex_id = *adjIt;

            // find originating state vertex
            graph_t::inv_adjacency_iterator revIt, revEnd;
            boost::tie(revIt, revEnd) = inv_adjacent_vertices(*vertexIt, *g);
            vertex_id_t originating_state_id = *revIt;

            // find target state vertex
            boost::tie(adjIt, adjEnd) = adjacent_vertices(command_vertex_id, *g);
            vertex_id_t target_state_id = *adjIt;

            // split condition
            condition_doc cond_b = comp_cond.condition_b;
            // assign first part to "original" condition
            vertex.contents = comp_cond.condition_a;
            // update the node's name
            hbc_printer pr;
            std::ostringstream cond_a_oss;
            pr(comp_cond.condition_a, cond_a_oss);
            vertex.name = cond_a_oss.str();

            // make new condtition vertex
            std::ostringstream cond_b_oss;
            pr(comp_cond.condition_b, cond_b_oss);
            vertex_id_t new_cond_vertex = add_vertex(g, cond_b_oss.str(), (*g)[originating_state_id].machine_id, ++max_vertex_id, v_cond);
            (*g)[new_cond_vertex].contents = comp_cond.condition_b;

            // make new command vertex -- its contents are a copy of the original's contents
            vertex_id_t new_cmd_vertex = add_vertex(g, (*g)[command_vertex_id].name, (*g)[originating_state_id].machine_id, ++max_vertex_id, v_command);
            (*g)[new_cmd_vertex].contents = (*g)[command_vertex_id].contents;

            // link vertices together
            add_edge(g, originating_state_id, new_cond_vertex, e_from_state);
            add_edge(g, new_cond_vertex, new_cmd_vertex, e_if_com);
            add_edge(g, new_cmd_vertex, target_state_id, e_to_state);

            i--; // don't increment index: we have to look at it again, in case it was a nested complex condition
          }
        }
      } catch (boost::bad_get b) {
        throw GraphSimplificationException("Condition vertex does not contain condition (during graph simplification)");
      }
    }
  }
}

void GraphSimplification::deleteOthersWrites(graph_t_ptr g, std::string name) {
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices(*g);
  for(; vertexIt != vertexEnd; vertexIt++) { // iterate over vertices of device's state machine
    vertex_t& vertex = (*g)[*vertexIt];

    if(vertex.type == v_command) {
      try {
        std::vector<command_doc>& cmds = boost::get<command_block_doc>(vertex.contents).commands;
        // delete command nodes for other devices
        for(unsigned int i = 0; i < cmds.size();/* increment only if nothing was deleted */) {
          command_doc cmd = cmds[i];
          try {
            if(boost::get<std::string>(cmd.write_command.geid.device_alias) != name)
              cmds.erase(cmds.begin() + i);
            else
              i++; // here we increment the index: Only if nothing was deleted. Otherwise we have to check the same index again because the elements are all moved one "to the left".
          } catch(boost::bad_get b) {
            throw InvalidPlaceholderException("Placeholder instead of device name found (during graph simplification)."); // This is an error during the graph transformation stage. A programmer's error is reported there
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

void GraphSimplification::operator()() {
  for(std::map<std::string, graph_t_ptr>::iterator it = _in_state_machines.begin(); it != _in_state_machines.end(); it++) { // iterate over list of device->state machine pairs (first pass)
    deleteOthersWrites(it->second, it->first); // delete write commands which are not for the device the stm. belongs to
  }

  for(std::map<std::string, graph_t_ptr>::iterator it = _in_state_machines.begin(); it != _in_state_machines.end(); it++) { // iterate over list of device->state machine pairs (second pass)
    expandComplexConditions(it->second);
  }

  for(std::map<std::string, graph_t_ptr>::iterator it = _in_state_machines.begin(); it != _in_state_machines.end(); it++) { // iterate over list of device->state machine pairs (second pass)
    expandMultipleWrites(it->second); // expand write nodes which (still) contain multiple write commands (because Hexabus Assembler only can do ONE write per transisiton
  }
}

