#include "graph_builder.hpp"
#include <sstream>
#include <boost/graph/graphviz.hpp>
#include <boost/utility.hpp>

using namespace hexabus;

static unsigned int _machine = 0; // unique IDs for the state machines

struct first_pass : boost::static_visitor<> {
  first_pass(graph_t_ptr graph) : _g(graph) { }

  void operator()(statemachine_doc& statemachine) const {
    // TODO maybe this is not okay once we actually bind the names..
    statemachine.id = _machine++;

    // don't use foreach because we need the index
    for(unsigned int i = 0; i < statemachine.stateset.states.size(); i++)
    {
      std::ostringstream oss;
      oss << "(" << statemachine.id << "." << i << ") " << statemachine.stateset.states[i];
      vertex_id_t v_id = boost::add_vertex((*_g));
      (*_g)[v_id].name = oss.str();
      (*_g)[v_id].machine_id = statemachine.id;
      (*_g)[v_id].state_id = i;
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

  boost::write_graphviz(os, (*_g),
    boost::make_label_writer(boost::get(&vertex_t::name, (*_g))),
    boost::make_label_writer(boost::get(&edge_t::name, (*_g))),
    boost::make_graph_attributes_writer(graph_attr, vertex_attr, edge_attr)
  );
}
