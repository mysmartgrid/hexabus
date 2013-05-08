#include "graph_builder.hpp"
#include <libhba/label_writer.hpp>
#include <libhba/graph.hpp>
#include <boost/foreach.hpp>
#include <boost/utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/copy.hpp>


using namespace hexabus;

static unsigned int _condition_id=0;
static unsigned int _state_id=0;

struct first_pass : boost::static_visitor<> {
  first_pass(graph_t_ptr graph) : _g(graph) { }

  void operator()(state_doc& hba_state) const
  {
    // issue an unique id
    hba_state.id = _state_id++;
    vertex_id_t v_id=boost::add_vertex((*_g));
    (*_g)[v_id].name = std::string(hba_state.name);
    (*_g)[v_id].lineno = hba_state.lineno;
    (*_g)[v_id].type = STATE;
    (*_g)[v_id].id = hba_state.id;
  }

  void operator()(condition_doc& hba_cond) const
  {
    hba_cond.id=_condition_id++;
    vertex_id_t v_id=boost::add_vertex((*_g));
    (*_g)[v_id].name = std::string(hba_cond.name);
    (*_g)[v_id].lineno = hba_cond.lineno;
    (*_g)[v_id].type = CONDITION;
    (*_g)[v_id].id = hba_cond.id;
  }

  graph_t_ptr _g;
};



struct second_pass : boost::static_visitor<> {
  second_pass(graph_t_ptr graph) : _g(graph)  { }

  void operator()(std::string const& from_state_name, if_clause_doc const& clause) const {
    bool ok;
    try {
      vertex_id_t from_state = find_vertex(_g, from_state_name);
      bool true_cond = false; // marks if the condition is "true" or a "proper" condition
      vertex_id_t condition;
      if(clause.name != "true")
        condition = find_vertex(_g, clause.name);
      else
        true_cond = true;
      vertex_id_t good_state = find_vertex(_g, clause.goodstate);
      vertex_id_t bad_state = find_vertex(_g, clause.badstate);

      if(!true_cond)
      {
        edge_id_t edge;
        boost::tie(edge, ok) = boost::add_edge(from_state, condition, (*_g));
        if (ok) {
          (*_g)[edge].type = FROM_STATE;
        } else {
          std::ostringstream oss;
          oss << "Cannot link state "
            << from_state_name << " with condition " << clause.name;
          throw EdgeLinkException(oss.str());
        }

        boost::tie(edge, ok) = boost::add_edge(condition, good_state, (*_g));
        if (ok) {
          (*_g)[edge].name=std::string("G");
          (*_g)[edge].type = GOOD_STATE;
        } else {
          std::ostringstream oss;
          oss << "Cannot link condition "
            << clause.name << " with good state " << clause.badstate<< std::endl;
          throw EdgeLinkException(oss.str());
        }

        boost::tie(edge, ok) = boost::add_edge(condition, bad_state, (*_g));
        if (ok) {
          (*_g)[edge].name=std::string("B");
          (*_g)[edge].type = BAD_STATE;
        } else {
          std::ostringstream oss;
          oss << "Cannot link condition "
            << clause.name << " with bad state " << clause.badstate<< std::endl;
          throw EdgeLinkException(oss.str());
        }
      } else // true_cond == true
      {
        edge_id_t edge;
        boost::tie(edge, ok) = boost::add_edge(from_state, good_state, (*_g));
        if(ok) {
          (*_g)[edge].name = std::string("T");
          (*_g)[edge].type = GOOD_STATE;
        } else {
          std::ostringstream oss;
          oss << "Cannot link state " << from_state_name << " with state " << clause.goodstate << " (true condition)." << std::endl;
          throw EdgeLinkException(oss.str());
        }
      }

    } catch (GenericException & ge) {
      std::cerr << ge.what() << " while processing clause "
        << clause.name << " of state " << from_state_name
        << " (line " << clause.lineno << ")"
        << std::endl << "Aborting." << std::endl;
      exit(-1);
    }
  }

  void operator()(state_doc const& hba_state) const
  {
    BOOST_FOREACH(if_clause_doc const& if_clause, hba_state.if_clauses)
    {
      second_pass p(_g);
      p(hba_state.name, if_clause);
    }
  }

  // We don't evaluate conditions here.
  void operator()(condition_doc const& hba) const
  { }

  graph_t_ptr _g;
};


void GraphBuilder::mark_start_state(const std::string& name) {
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for (; vertexIt != vertexEnd; ++vertexIt){
    vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
    vertex_t & vertex = (*_g)[vertexID];
    if (name == vertex.name) {
      vertex.type=STARTSTATE;
      return;
    }
  }
  // we have not found a vertex id.
  std::ostringstream oss;
  if(name == "") { // No start state name was given in the source code
    oss << "no start state defined in input file.";
  } else { // the start state name was not found in the state set
    oss << "state " << name << " not found.";
  }
  throw VertexNotFoundException(oss.str());
}


void GraphBuilder::operator()(hba_doc& hba) {
  // 1st pass: grab all the states from the hba_doc
  BOOST_FOREACH(hba_doc_block& block, hba.blocks)
  {
    boost::apply_visitor(first_pass(_g), block);
  }
  // pass 1.1: mark the start state (which should be defined now)
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  if(std::distance(vertexIt, vertexEnd) != 0) // only mark start state if state set is not empty
  {
    try {
      mark_start_state(hba.start_state);
    } catch (VertexNotFoundException& vnfe) {
      std::cout << "ERROR: cannot assign start state, " << vnfe.what() << std::endl;
    }
    // 2nd pass: now add the edges in the second pass
    BOOST_FOREACH(hba_doc_block& block, hba.blocks)
    {
      boost::apply_visitor(second_pass(_g), block);
    }
  }
}


void GraphBuilder::write_graphviz(std::ostream& os) {
  std::map<std::string,std::string> graph_attr, vertex_attr, edge_attr;
  //graph_attr["size"] = "3,3";
  // graph_attr["rankdir"] = "LR";
  graph_attr["ratio"] = "fill";
  vertex_attr["shape"] = "circle";

  //boost::dynamic_properties dp;
  //dp.property("label", boost::get(&vertex_t::name, (*_g)));
  //dp.property("node_id", boost::get(boost::vertex_index, (*_g)));
  //dp.property("label", boost::get(&edge_t::name, (*_g)));

  // graph_t::vertex_iterator vertexIt, vertexEnd;
  // boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  // for (; vertexIt != vertexEnd; ++vertexIt){
  //   vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
  //   make_state_cond_label_writer(
  //   	boost::get(&vertex_t::name, (*_g)),
  //   	boost::get(&vertex_t::type, (*_g))
  //     )(std::cout, vertexID);
  //   std::cout << std::endl;
  // }


  boost::write_graphviz(os, (*_g),
      //boost::make_label_writer(boost::get(&vertex_t::name, (*_g))), //_names[0]),
    make_state_cond_label_writer(
        boost::get(&vertex_t::name, (*_g)),
        boost::get(&vertex_t::id, (*_g)),
        boost::get(&vertex_t::type, (*_g))),
    //boost::make_label_writer(boost::get(&edge_t::name, (*_g))), //_names[0]),
    make_edge_label_writer(
        boost::get(&edge_t::name, (*_g)),
        boost::get(&edge_t::type, (*_g))),
    boost::make_graph_attributes_writer(graph_attr, vertex_attr, edge_attr)
      );
}

