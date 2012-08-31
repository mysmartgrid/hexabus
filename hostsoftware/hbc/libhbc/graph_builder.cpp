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
      vertex_id_t v_id = boost::add_vertex((*_g));
      (*_g)[v_id].name = oss.str();
      (*_g)[v_id].machine_id = statemachine.id;
      (*_g)[v_id].vertex_id = i;
      (*_g)[v_id].type = v_state;
    }

    // check if init state is present
    try {
      find_state_vertex_id(_g, statemachine, "init");
    } catch(StateNameNotFoundException e) {
      std::ostringstream oss;
      oss << "[" << statemachine.read_from_file << ":" << statemachine.lineno << "] Missing init state." << std::endl;
      throw NonexistentStateException(oss.str());
    }

    // build the edges
    // TODO The following 200 lines or so need to be despaghettified. a lot.
    BOOST_FOREACH(in_clause_doc in_clause, statemachine.in_clauses) {
      unsigned int condition_id = 0; // unique (per-state-machine) index of condition vertex
      unsigned int command_id = 0;   // per-state-machine unique IDs for command blocks
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
        vertex_id_t v_id = boost::add_vertex((*_g));
        (*_g)[v_id].name = oss.str();
        (*_g)[v_id].machine_id = statemachine.id;
        (*_g)[v_id].vertex_id = condition_id++;
        (*_g)[v_id].type = v_cond;

        // add edges
        // edge from from-state to condition vertex
        vertex_id_t from_state = find_vertex(_g, statemachine.id, originating_state);
        edge_id_t edge;
        bool if_ok;
        boost::tie(edge, if_ok) = boost::add_edge(from_state, v_id, (*_g));
        if(if_ok)
          (*_g)[edge].type = e_from_state;
        else {
          std::ostringstream oss;
          oss << "Cannot link state " << statemachine.id << "." << originating_state << " to condition " << (*_g)[v_id].vertex_id;
          throw EdgeLinkException(oss.str());
        }

        // make a command-block vertex
        std::ostringstream c_oss;
        pr(if_clause.if_block.command_block, c_oss);
        // remove linebreaks
        std::string name_str = c_oss.str();
        size_t found = name_str.find("\n");
        while(found != std::string::npos) {
          name_str.replace(found, 1, "\\n");
          found = name_str.find("\n");
        }
        vertex_id_t if_command_v_id = boost::add_vertex((*_g));
        (*_g)[if_command_v_id].name = name_str;
        (*_g)[if_command_v_id].machine_id = statemachine.id;
        (*_g)[if_command_v_id].vertex_id = command_id++;
        (*_g)[if_command_v_id].type = v_command;
        // edge from condition vertex to command block vertex
        edge_id_t if_com_edge;
        bool if_com_ok;
        boost::tie(if_com_edge, if_com_ok) = boost::add_edge(v_id, if_command_v_id, (*_g));
        if(if_com_ok)
          (*_g)[if_com_edge].type = e_if_com;
        else {
          std::ostringstream oss;
          oss << "Cannot link condition " << (*_g)[v_id].vertex_id  << " to command block " << (*_g)[if_command_v_id].vertex_id;
          throw EdgeLinkException(oss.str());
        }

        // edge from command vertex to to-state
        vertex_id_t to_state = find_vertex(_g, statemachine.id, target_state);
        edge_id_t to_edge;
        bool to_ok;
        boost::tie(to_edge, to_ok) = boost::add_edge(if_command_v_id, to_state, (*_g));
        if(to_ok)
          (*_g)[edge].type = e_to_state;
        else {
          std::ostringstream oss;
          oss << "Cannot link condition " << statemachine.id << "." << (*_g)[v_id].vertex_id << " to state " << to_state;
          throw EdgeLinkException(oss.str());
        }

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
          vertex_id_t v_id = boost::add_vertex((*_g));
          (*_g)[v_id].name = oss.str();
          (*_g)[v_id].machine_id = statemachine.id;
          (*_g)[v_id].vertex_id = condition_id++;
          (*_g)[v_id].type = v_cond;

          // from-state to condition vertex
          vertex_id_t from_state = find_vertex(_g, statemachine.id, originating_state);
          edge_id_t edge;
          bool ok;
          boost::tie(edge, ok) = boost::add_edge(from_state, v_id, (*_g));
          if(ok)
            (*_g)[edge].type = e_from_state;
          else {
            std::ostringstream oss;
            oss << "Cannot link state " << statemachine.id << "." << originating_state << " to condition " << (*_g)[v_id].vertex_id;
            throw EdgeLinkException(oss.str());
          }

          // make a command-block vertex
          std::ostringstream c_oss;
          pr(else_if_block.command_block, c_oss);
          // remove linebreaks
          std::string name_str = c_oss.str();
          size_t found = name_str.find("\n");
          while(found != std::string::npos) {
            name_str.replace(found, 1, "\\n");
            found = name_str.find("\n");
          }
          vertex_id_t command_v_id = boost::add_vertex((*_g));
          (*_g)[command_v_id].name = name_str;
          (*_g)[command_v_id].machine_id = statemachine.id;
          (*_g)[command_v_id].vertex_id = command_id++;
          (*_g)[command_v_id].type = v_command;
          // edge from condition vertex to command block vertex
          edge_id_t if_com_edge;
          bool if_com_ok;
          boost::tie(if_com_edge, if_com_ok) = boost::add_edge(v_id, command_v_id, (*_g));
          if(if_com_ok)
            (*_g)[if_com_edge].type = e_if_com;
          else {
            std::ostringstream oss;
            oss << "Cannot link condition " << (*_g)[v_id].vertex_id  << " to command block " << (*_g)[command_v_id].vertex_id;
            throw EdgeLinkException(oss.str());
          }

          vertex_id_t to_state = find_vertex(_g, statemachine.id, target_state);
          edge_id_t to_edge;
          bool to_ok;
          boost::tie(to_edge, to_ok) = boost::add_edge(command_v_id, to_state, (*_g));
          if(ok)
            (*_g)[edge].type = e_to_state;
          else {
            std::ostringstream oss;
            oss << "Cannot link condition " << statemachine.id << "." << (*_g)[v_id].vertex_id << " to state " << to_state;
            throw EdgeLinkException(oss.str());
          }
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
          vertex_id_t else_v_id = boost::add_vertex((*_g));
          (*_g)[else_v_id].name = else_oss.str();
          (*_g)[else_v_id].machine_id = statemachine.id;
          (*_g)[else_v_id].vertex_id = condition_id++;
          (*_g)[else_v_id].type = v_cond;

          // add edges
          // edge from from-state to condition vertex
          vertex_id_t else_from_state = find_vertex(_g, statemachine.id, originating_state);
          edge_id_t else_edge;
          bool else_ok;
          boost::tie(else_edge, else_ok) = boost::add_edge(else_from_state, else_v_id, (*_g));
          if(if_ok)
            (*_g)[else_edge].type = e_from_state;
          else {
            std::ostringstream oss;
            oss << "Cannot link state " << statemachine.id << "." << originating_state << " to condition " << (*_g)[else_v_id].vertex_id;
            throw EdgeLinkException(oss.str());
          }
          // make a command-block vertex
          std::ostringstream else_c_oss;
          pr(if_clause.else_clause.commands, else_c_oss);
          // remove linebreaks
          std::string else_name_str = else_c_oss.str();
          size_t found = else_name_str.find("\n");
          while(found != std::string::npos) {
            else_name_str.replace(found, 1, "\\n");
            found = else_name_str.find("\n");
          }
          vertex_id_t else_command_v_id = boost::add_vertex((*_g));
          (*_g)[else_command_v_id].name = else_name_str;
          (*_g)[else_command_v_id].machine_id = statemachine.id;
          (*_g)[else_command_v_id].vertex_id = command_id++;
          (*_g)[else_command_v_id].type = v_command;
          // edge from condition vertex to command block vertex
          edge_id_t else_com_edge;
          bool else_com_ok;
          boost::tie(else_com_edge, else_com_ok) = boost::add_edge(else_v_id, else_command_v_id, (*_g));
          if(else_com_ok)
            (*_g)[else_com_edge].type = e_if_com;
          else {
            std::ostringstream oss;
            oss << "Cannot link condition " << (*_g)[v_id].vertex_id  << " to command block " << (*_g)[else_command_v_id].vertex_id;
            throw EdgeLinkException(oss.str());
          }

          vertex_id_t else_to_state = find_vertex(_g, statemachine.id, else_target_state);
          edge_id_t else_to_edge;
          bool else_to_ok;
          boost::tie(else_to_edge, else_to_ok) = boost::add_edge(else_command_v_id, else_to_state, (*_g));
          if(else_to_ok)
            (*_g)[else_to_edge].type = e_to_state;
          else {
            std::ostringstream oss;
            oss << "Cannot link condition " << statemachine.id << "." << (*_g)[v_id].vertex_id << " to state " << to_state;
            throw EdgeLinkException(oss.str());
          }
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
