#include "graph_builder.hpp"
#include <sstream>
#include <boost/graph/graphviz.hpp>

using namespace hexabus;

static unsigned int _state_id = 0;
static unsigned int _condition = 0;

struct first_pass : boost::static_visitor<> {
  first_pass(graph_t_ptr graph) : _g(graph) { }

  void operator()(statemachine_doc& statemachine) const {
    // TODO maybe this is not okay once we actually bind the names..
    BOOST_FOREACH(std::string state, statemachine.stateset.states) {
      vertex_id_t v_id = boost::add_vertex((*_g));
      (*_g)[v_id].name = state;
      (*_g)[v_id].id = _state_id++; // TODO make a state struct inside the AST and give this object the ID, so we can find it later!!
      (*_g)[v_id].type = v_state;
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
  vertex_attr["shape"] = "circle";

  boost::write_graphviz(os, (*_g) // ,
    // boost::make_label_writer(boost::get(&vertex_t::name, (*_g)))m
    // boost::make_label_writer(boost::get(&edge_t::name, (*_g)))
    // boost::make_graph_attributes_writer(graph_attr, vertex_attr, edge_attr)
  );
}
