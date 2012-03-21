#include "graph_builder.hpp"
#include <libhba/label_writer.hpp>
#include <boost/foreach.hpp>
#include <boost/utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/copy.hpp>


using namespace hexabus;

static graph_t graph;

struct first_pass : boost::static_visitor<> {
  first_pass(graph_t_ptr graph) : _g(graph) { }

  void operator()(state_doc const& hba_state) const
  {
	//std::cout << "state: " << hba_state.name << " (line "
	//  << hba_state.lineno << ")" << std::endl;
	//std::cout << '{' << std::endl;
	vertex_id_t v_id=boost::add_vertex((*_g));
	(*_g)[v_id].name = std::string(hba_state.name);
	(*_g)[v_id].lineno = hba_state.lineno;
	(*_g)[v_id].type = STATE;
  }

  void operator()(condition_doc const& hba_cond) const
  {
	vertex_id_t v_id=boost::add_vertex((*_g));
	(*_g)[v_id].name = std::string(hba_cond.name);
	(*_g)[v_id].lineno = hba_cond.lineno;
	(*_g)[v_id].type = CONDITION;
  }

  graph_t_ptr _g;

};



struct second_pass : boost::static_visitor<> {
  second_pass(graph_t_ptr graph) : _g(graph)  { }

  vertex_id_t find_vertex(const std::string& name) const {
	graph_t::vertex_iterator vertexIt, vertexEnd;
	boost::tie(vertexIt, vertexEnd) = vertices((*_g));
	for (; vertexIt != vertexEnd; ++vertexIt){
	  vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
	  vertex_t & vertex = (*_g)[vertexID];
	  if (name == vertex.name) {
		return vertexID;
	  } 
	}
	// we have not found an vertex id.
	std::ostringstream oss;
	oss << "cannot find state " << name;
	throw VertexNotFoundException(oss.str());
  }


  void operator()(std::string const& from_state_name, if_clause_doc const& clause) const {
	bool ok;
	try {
	  vertex_id_t from_state = find_vertex(from_state_name);
	  vertex_id_t condition = find_vertex(clause.name);
	  vertex_id_t good_state = find_vertex(clause.goodstate);
	  vertex_id_t bad_state = find_vertex(clause.badstate);

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
	// we have not found an vertex id.
	std::ostringstream oss;
	oss << "state " << name << " not found.";
	throw VertexNotFoundException(oss.str());
}


// TODO: Refactoring, move to separate class
void GraphBuilder::check_states_incoming() const {
  // an unreachable state has no incoming edge. So, iterate over
  // all vertices and compute the number of incoming edges. Raise exception
  // if an vertex has no incoming edges, i.e. is unreachable.

  // 1. iterate over all vertices.
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for (; vertexIt != vertexEnd; ++vertexIt){
	vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
	vertex_t & vertex = (*_g)[vertexID];
	// if this vertex is a state - ignore conditions
	if (vertex.type == STATE || vertex.type == STARTSTATE) {
	  // 2. Check number of incoming edges.
	  graph_t::inv_adjacency_iterator inedgeIt, inedgeEnd;
	  boost::tie(inedgeIt, inedgeEnd) = inv_adjacent_vertices(vertexID, (*_g));
	  if (std::distance(inedgeIt, inedgeEnd) == 0) {
		std::ostringstream oss;
		oss << "State " << vertex.name << " (line " 
		  << vertex.lineno << ") is not reachable." << std::endl;
		throw UnreachableException(oss.str());
	  }
	}
  }
}

void GraphBuilder::check_states_outgoing() const {
  // an end state has no outgoing edges. So, iterate over
  // all vertices and compute the number of outgoing edges. Raise exception
  // if an vertex has no outgoing edges, i.e. is an endstate.

  // 1. iterate over all vertices.
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for (; vertexIt != vertexEnd; ++vertexIt){
	vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
	vertex_t & vertex = (*_g)[vertexID];
	// if this vertex is a state - ignore conditions
	if (vertex.type == STATE || vertex.type == STARTSTATE) {
	  // 2. Check number of outgoing edges.
	  graph_t::adjacency_iterator inedgeIt, inedgeEnd;
	  boost::tie(inedgeIt, inedgeEnd) = adjacent_vertices(vertexID, (*_g));
	  if (std::distance(inedgeIt, inedgeEnd) == 0) {
		std::ostringstream oss;
		oss << "State " << vertex.name << " (line " 
		  << vertex.lineno << ") cannot be left." << std::endl;
		throw UnleavableException(oss.str());
	  }
	}
  }
}

void GraphBuilder::check_conditions_incoming() const {
  // an unreachable condition has no incoming edge. So, iterate over
  // all vertices and compute the number of incoming edges. Raise exception
  // if a condition has no incoming edges, i.e. is unreachable.

  // 1. iterate over all vertices.
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for (; vertexIt != vertexEnd; ++vertexIt){
	vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
	vertex_t & vertex = (*_g)[vertexID];
	// if this vertex is a condition
	if (vertex.type == CONDITION) {
	  // 2. Check number of incoming edges.
	  graph_t::inv_adjacency_iterator inedgeIt, inedgeEnd;
	  boost::tie(inedgeIt, inedgeEnd) = inv_adjacent_vertices(vertexID, (*_g));
	  if (std::distance(inedgeIt, inedgeEnd) == 0) {
		std::ostringstream oss;
		oss << "Condition " << vertex.name << " (line " 
		  << vertex.lineno << ") is not reachable." << std::endl;
		throw UnreachableException(oss.str());
	  }
	}
  }
}

void GraphBuilder::check_conditions_outgoing() const {
  // an end condition has no outgoing edges. So, iterate over
  // all vertices and compute the number of outgoing edges. Raise exception
  // if an vertex has not two outgoing edges, i.e. goodstate and badstate 
  // are missing.

  // 1. iterate over all vertices.
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices((*_g));
  for (; vertexIt != vertexEnd; ++vertexIt){
	vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
	vertex_t & vertex = (*_g)[vertexID];
	// if this vertex is a condition
	if (vertex.type == CONDITION) {
	  // 2. Check number of outgoing edges.
	  graph_t::adjacency_iterator inedgeIt, inedgeEnd;
	  boost::tie(inedgeIt, inedgeEnd) = adjacent_vertices(vertexID, (*_g));
	  size_t num_follow_states=std::distance(inedgeIt, inedgeEnd);
	  if (num_follow_states != 2) {
		std::ostringstream oss;
		oss << "Condition " << vertex.name << " (line " 
		  << vertex.lineno << ") has " << num_follow_states 
		  << " following states - expected two." << std::endl;
		oss << "Follow-up states are:" << std::endl;
		for (; inedgeIt != inedgeEnd; ++inedgeIt) {
		  vertex_id_t f_vertexID = *inedgeIt; // dereference vertexIt, get the ID
		  vertex_t & f_vertex = (*_g)[f_vertexID];
		  oss << "- " << f_vertex.name << std::endl;
		}
		throw UnleavableException(oss.str());
	  }
	}
  }
}



void GraphBuilder::operator()(hba_doc const& hba) {
  // 1st pass: grab all the states from the hba_doc
  BOOST_FOREACH(hba_doc_block const& block, hba.blocks)
  {
	boost::apply_visitor(first_pass(_g), block);
  }
  // pass 1.1: mark the start state (which should be defined now)
  try {
	mark_start_state(hba.start_state);
  } catch (VertexNotFoundException& vnfe) {
	std::cout << "ERROR: cannot assign start state, " << vnfe.what() << std::endl;
  }
  // 2nd pass: now add the edges in the second pass
  BOOST_FOREACH(hba_doc_block const& block, hba.blocks)
  {
	boost::apply_visitor(second_pass(_g), block);
  }
  try {
	check_states_incoming();
	check_states_outgoing();
	check_conditions_incoming();
	check_conditions_outgoing();
  } catch (GenericException& ge) {
	std::cout << "ERROR: " << ge.what() << std::endl;
	exit(-1);
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
		boost::get(&vertex_t::type, (*_g))),
	//boost::make_label_writer(boost::get(&edge_t::name, (*_g))), //_names[0]),
	make_edge_label_writer(
		boost::get(&edge_t::name, (*_g)),
		boost::get(&edge_t::type, (*_g))),
	boost::make_graph_attributes_writer(graph_attr, vertex_attr, edge_attr)
	  );
}


