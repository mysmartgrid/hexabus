#include "graph_builder.hpp"
#include <libhbc/hbc_printer.hpp>
#include <sstream>
#include <boost/graph/graphviz.hpp>
#include <boost/utility.hpp>

using namespace hexabus;

static unsigned int _machine = 0; // unique IDs for the state machines

struct first_pass : boost::static_visitor<> {
  first_pass(graph_t_ptr graph) : _g(graph) { }

  void operator()(statemachine_doc& statemachine) const {
    statemachine.id = _machine++;

    // build vertices for states
    for(unsigned int i = 0; i < statemachine.stateset.states.size(); i++) // don't use foreach because we need the index
    {
      std::ostringstream oss;
      oss << "(" << statemachine.id << "." << i << ") " << statemachine.stateset.states[i];
      vertex_id_t v_id = boost::add_vertex((*_g));
      (*_g)[v_id].name = oss.str();
      (*_g)[v_id].machine_id = statemachine.id;
      (*_g)[v_id].vertex_id = i;
      (*_g)[v_id].type = v_state;
    }

    // build the edges
    BOOST_FOREACH(in_clause_doc in_clause, statemachine.in_clauses) {
      unsigned int condition_id = 0; // unique (per-state-machine) index of condition vertex
      // find originating state ID
      unsigned int originating_state = find_state_vertex_id(_g, statemachine, in_clause.name);
      // look at all the "if"s in the in_clause, and find their goto's.
      BOOST_FOREACH(if_clause_doc if_clause, in_clause.if_clauses) {
        // if-block
        unsigned int target_state = find_state_vertex_id(_g, statemachine, if_clause.if_block.command_block.goto_command.target_state);
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
          // TODO throw EdgeLinkException(oss);
        }
        // edge from condition vertex to to-state
        vertex_id_t to_state = find_vertex(_g, statemachine.id, target_state);
        edge_id_t to_edge;
        bool to_ok;
        boost::tie(to_edge, to_ok) = boost::add_edge(v_id, to_state, (*_g));
        if(to_ok)
          (*_g)[edge].type = e_to_state;
        else {
          std::ostringstream oss;
          oss << "Cannot link condition " << statemachine.id << "." << (*_g)[v_id].vertex_id << " to state " << to_state;
          // TODO throw EdgeLinkExgeption
        }

        // else-if-block(s)
        BOOST_FOREACH(guarded_command_block_doc else_if_block, if_clause.else_if_blocks) {
          unsigned int target_state = find_state_vertex_id(_g, statemachine, else_if_block.command_block.goto_command.target_state);
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

          vertex_id_t from_state = find_vertex(_g, statemachine.id, originating_state);
          edge_id_t edge;
          bool ok;
          boost::tie(edge, ok) = boost::add_edge(from_state, v_id, (*_g));
          if(ok)
            (*_g)[edge].type = e_from_state;
          else {
            std::ostringstream oss;
            oss << "Cannot link state " << statemachine.id << "." << originating_state << " to condition " << (*_g)[v_id].vertex_id;
            // TODO throw EdgeLinkException(oss);
          }

          vertex_id_t to_state = find_vertex(_g, statemachine.id, target_state);
          edge_id_t to_edge;
          bool to_ok;
          boost::tie(to_edge, to_ok) = boost::add_edge(v_id, to_state, (*_g));
          if(ok)
            (*_g)[edge].type = e_to_state;
          else {
            std::ostringstream oss;
            oss << "Cannot link condition " << statemachine.id << "." << (*_g)[v_id].vertex_id << " to state " << to_state;
            // TODO throw EdgeLinkExgeption
          }
        }

        // else-block (TODO if it exists)
        unsigned int else_target_state = find_state_vertex_id(_g, statemachine, if_clause.else_block.goto_command.target_state);
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
          // TODO throw EdgeLinkException(oss);
        }
        // else_edge from condition vertex to to-state
        vertex_id_t else_to_state = find_vertex(_g, statemachine.id, else_target_state);
        edge_id_t else_to_edge;
        bool else_to_ok;
        boost::tie(else_to_edge, else_to_ok) = boost::add_edge(else_v_id, else_to_state, (*_g));
        if(else_to_ok)
          (*_g)[else_edge].type = e_to_state;
        else {
          std::ostringstream oss;
          oss << "Cannot link condition " << statemachine.id << "." << (*_g)[else_v_id].vertex_id << " to state " << else_to_state;
          // TODO throw EdgeLinkExgeption
        }
      }
    }
  }

  void operator()(include_doc& include) const { }
  void operator()(endpoint_doc& ep) const { }
  void operator()(alias_doc alias) const { }
  void operator()(module_doc module) const { }

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
    boost::make_label_writer(boost::get(&vertex_t::name, (*_g))),
    boost::make_label_writer(boost::get(&edge_t::name, (*_g))),
    boost::make_graph_attributes_writer(graph_attr, vertex_attr, edge_attr)
  );
}
