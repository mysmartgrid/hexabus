#include "graph_builder.hpp"
#include <libhbc/hbc_printer.hpp>
#include <libhbc/label_writer.hpp>
#include <sstream>
#include <boost/graph/graphviz.hpp>
#include <boost/utility.hpp>
#include <libhbc/error.hpp>

using namespace hexabus;

static unsigned int _machine = 0; // unique IDs for the state machines

struct first_pass : boost::static_visitor<> {
  first_pass(graph_t_ptr graph) : _g(graph) { }

  void operator()(statemachine_doc& statemachine) const {
    statemachine.id = _machine++;

    // build vertices for states
    for(unsigned int i = 0; i < statemachine.stateset.states.size(); i++) { // don't use foreach because we need the index
      std::ostringstream oss;
      oss << "(" << statemachine.id << "." << i << ") \\n" << statemachine.name << "." <<  statemachine.stateset.states[i];

      add_vertex(_g, oss.str(), statemachine.id, i, v_state);
    }

    // check if init state is present
    try {
      find_state_vertex_id(_g, statemachine, "init");
    } catch(StateNameNotFoundException e) {
      std::ostringstream oss;
      oss << "[" << statemachine.read_from_file << ":" << statemachine.lineno << "] Missing init state." << std::endl;
      throw NonexistentStateException(oss.str());
    }

    unsigned int condition_id = 0; // unique (per-state-machine) index of condition vertex
    unsigned int command_id = 0;   // per-state-machine unique IDs for command blocks

    // build the edges
    BOOST_FOREACH(in_clause_doc in_clause, statemachine.in_clauses) {
      // find originating state ID
      unsigned int originating_state;
      try {
        originating_state = find_state_vertex_id(_g, statemachine, in_clause.name);
      } catch(StateNameNotFoundException e) {
        std::ostringstream oss;
        oss << "[" << statemachine.read_from_file << ":" << in_clause.lineno << "] In clause from nonexistent state " << e.what() << "." << std::endl;
        throw NonexistentStateException(oss.str());
      }
      // look at all the "if"s in the in_clause, and find their goto's.
      BOOST_FOREACH(if_clause_doc if_clause, in_clause.if_clauses) {
        // if-block
        unsigned int target_state;
        try {
          target_state = find_state_vertex_id(_g, statemachine, if_clause.if_block.command_block.goto_command.target_state);
        } catch(StateNameNotFoundException e) {
          std::ostringstream oss;
          oss << "[" << statemachine.read_from_file << ":" << if_clause.if_block.command_block.goto_command.lineno << "] Goto to nonexistent state. " << e.what() << "." << std::endl;
          throw NonexistentStateException(oss.str());
        }
        // add condition vertex
        std::ostringstream oss;
        oss << "(" << condition_id << ") if (";
        hbc_printer pr;
        pr(if_clause.if_block.condition, oss);
        oss << ")";
        vertex_id_t v_id = add_vertex(_g, oss.str(), statemachine.id, condition_id++, v_cond, if_clause.if_block.condition);

        // add edges
        // edge from from-state to condition vertex
        vertex_id_t from_state = find_vertex(_g, statemachine.id, originating_state);
        add_edge(_g, from_state, v_id, e_from_state);

        // make a command-block vertex
        std::ostringstream c_oss;
        pr(if_clause.if_block.command_block, c_oss);
        vertex_id_t if_command_v_id = add_vertex(_g, c_oss.str(), statemachine.id, command_id++, v_command, if_clause.if_block.command_block);

        // edge from condition vertex to command block vertex
        add_edge(_g, v_id, if_command_v_id, e_if_com);

        // edge from command vertex to to-state
        vertex_id_t to_state = find_vertex(_g, statemachine.id, target_state);
        add_edge(_g, if_command_v_id, to_state, e_to_state);

        // else-if-block(s)
        BOOST_FOREACH(guarded_command_block_doc else_if_block, if_clause.else_if_blocks) {
          unsigned int target_state;
          try {
            target_state = find_state_vertex_id(_g, statemachine, else_if_block.command_block.goto_command.target_state);
          } catch(StateNameNotFoundException e) {
            std::ostringstream oss;
            oss << "[" << statemachine.read_from_file << ":" << else_if_block.command_block.goto_command.lineno << "] Goto to nonexistent state. " << e.what() << "." << std::endl;
            throw NonexistentStateException(oss.str());
          }
          std::ostringstream oss;
          oss << "(" << condition_id << ") else if (";
          hbc_printer pr;
          pr(else_if_block.condition, oss);
          oss << ")";
          vertex_id_t v_id = add_vertex(_g, oss.str(), statemachine.id, condition_id++, v_cond, else_if_block.condition);

          // from-state to condition vertex
          vertex_id_t from_state = find_vertex(_g, statemachine.id, originating_state);
          add_edge(_g, from_state, v_id, e_from_state);

          // make a command-block vertex
          std::ostringstream c_oss;
          pr(else_if_block.command_block, c_oss);
          vertex_id_t command_v_id = add_vertex(_g, c_oss.str(), statemachine.id, command_id++, v_command, else_if_block.command_block);

          // edge from condition vertex to command block vertex
          add_edge(_g, v_id, command_v_id, e_if_com);

          vertex_id_t to_state = find_vertex(_g, statemachine.id, target_state);
          add_edge(_g, command_v_id, to_state, e_to_state);
        }

        // else-block
        if(if_clause.else_clause.present == 1) {
          unsigned int else_target_state;
          try {
            else_target_state = find_state_vertex_id(_g, statemachine, if_clause.else_clause.commands.goto_command.target_state);
          } catch(StateNameNotFoundException e) {
            std::ostringstream oss;
            oss << "[" << statemachine.read_from_file << ":" << if_clause.else_clause.commands.goto_command.lineno << "] Goto to nonexistent state. " << e.what() << std::endl;
            throw NonexistentStateException(oss.str());
          }
          // add condition vertex
          std::ostringstream else_oss;
          else_oss << "(" << condition_id << ") else";
          vertex_id_t else_v_id = add_vertex(_g, else_oss.str(), statemachine.id, condition_id++, v_cond /* TODO contents */);

          // add edges
          // edge from from-state to condition vertex
          vertex_id_t else_from_state = find_vertex(_g, statemachine.id, originating_state);
          add_edge(_g, else_from_state, else_v_id, e_from_state);

          // make a command-block vertex
          std::ostringstream else_c_oss;
          pr(if_clause.else_clause.commands, else_c_oss);
          vertex_id_t else_command_v_id = add_vertex(_g, else_c_oss.str(), statemachine.id, command_id++, v_command, if_clause.else_clause.commands);

          add_edge(_g, else_v_id, else_command_v_id, e_if_com);

          vertex_id_t else_to_state = find_vertex(_g, statemachine.id, else_target_state);
          add_edge(_g, else_command_v_id, else_to_state, e_to_state);
        }
      }
    }
  }

  void operator()(include_doc& include) const { }
  void operator()(endpoint_doc& ep) const { }
  void operator()(alias_doc& alias) const { }
  void operator()(module_doc& module) const { }
  void operator()(instantiation_doc& inst) const { }

  graph_t_ptr _g;
};

void GraphBuilder::operator()(hbc_doc& hbc) {
  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    // only state machines
    boost::apply_visitor(first_pass(_g), block);
  }

  // TODO moar passes
}

void GraphBuilder::write_graphviz(std::ostream& os) {
  std::map<std::string, std::string> graph_attr, vertex_attr, edge_attr;
  graph_attr["ratio"] = "fill";
  vertex_attr["shape"] = "rectangle";

  boost::write_graphviz(os, (*_g),
    make_vertex_label_writer(
      boost::get(&vertex_t::name, (*_g)), boost::get(&vertex_t::type, (*_g))),
      boost::make_label_writer(boost::get(&edge_t::name, (*_g))),
      boost::make_graph_attributes_writer(graph_attr, vertex_attr, edge_attr
    )
  );
}
