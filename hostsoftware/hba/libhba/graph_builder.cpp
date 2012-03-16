#include "graph_builder.hpp"
#include <boost/foreach.hpp>
#include <boost/utility.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/depth_first_search.hpp>


using namespace hexabus;

static graph_t graph;

struct first_pass : boost::static_visitor<> {
  first_pass() { }

  void operator()(state_doc const& hba_state) const
  {
	//std::cout << "state: " << hba_state.name << " (line "
	//  << hba_state.lineno << ")" << std::endl;
	//std::cout << '{' << std::endl;
	vertex_id_t v_id=boost::add_vertex(graph);
	graph[v_id].name = std::string(hba_state.name);
  }

  // we do not evaluate the conditions.
  void operator()(condition_doc const& hba) const
  {}


};



struct second_pass : boost::static_visitor<> {
  second_pass()  { }

  vertex_id_t find_vertex(const std::string& name) const {
	graph_t::vertex_iterator vertexIt, vertexEnd;
	boost::tie(vertexIt, vertexEnd) = vertices(graph);
	for (; vertexIt != vertexEnd; ++vertexIt){
	  vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
	  vertex_t & vertex = graph[vertexID];
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
	  vertex_id_t good_state = find_vertex(clause.goodstate);
	  vertex_id_t bad_state = find_vertex(clause.badstate);

	  edge_id_t edge;
	  boost::tie(edge, ok) = boost::add_edge(from_state, good_state, graph);
	  if (ok)
		graph[edge].name=std::string("G:")+clause.name;
	  else std::cout << "Cannot link states " 
		<< from_state_name << " and " << clause.goodstate << std::endl;

	  boost::tie(edge, ok) = boost::add_edge(from_state, bad_state, graph);
	  if (ok)
		graph[edge].name=std::string("B:")+clause.name;
	  else std::cout << "Cannot link states " 
		<< from_state_name << " and " << clause.badstate<< std::endl;
	} catch (VertexNotFoundException& vnfe) {
	  std::cerr << vnfe.what() << " while processing clause "
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
	  second_pass p;
	  p(hba_state.name, if_clause);
	}
  }

  // We don't evaluate conditions.
  void operator()(condition_doc const& hba) const
  { }

};


// TODO: Refactoring, move to separate class
void GraphBuilder::check_unreachable_states() const {
  // an unreachable state has no incoming edge. So, iterate over
  // all vertices and compute the number of incoming edges. Raise exception
  // if an vertex has no incoming edges, i.e. is unreachable.

  // 1. iterate over all vertices.
  graph_t::vertex_iterator vertexIt, vertexEnd;
  boost::tie(vertexIt, vertexEnd) = vertices(graph);
  for (; vertexIt != vertexEnd; ++vertexIt){
	vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
	vertex_t & vertex = graph[vertexID];

	// 2. Check number of incoming edges.
//	graph_t::in_edge_iterator inedgeIt, inedgeEnd;
//	boost::tie(inedgeIt, inedgeEnd) = 
//	  in_edges(vertexID, graph);
	graph_t::inv_adjacency_iterator inedgeIt, inedgeEnd;
    boost::tie(inedgeIt, inedgeEnd) = inv_adjacent_vertices(vertexID, graph);
	if (inedgeIt == inedgeEnd) {
	  std::ostringstream oss;
	  oss << "State " << vertex.name << " is not reachable." << std::endl;
	  throw UnreachableStateException(oss.str());
	}
  }
}

void GraphBuilder::operator()(hba_doc const& hba) const {
  // 1st pass: grab all the states from the hba_doc
  BOOST_FOREACH(hba_doc_block const& block, hba.blocks)
  {
	boost::apply_visitor(first_pass(), block);
  }
  // 2nd pass: now add the edges in the second pass
  BOOST_FOREACH(hba_doc_block const& block, hba.blocks)
  {
	boost::apply_visitor(second_pass(), block);
  }
  try {
  check_unreachable_states();
  } catch (UnreachableStateException& use) {
	std::cout << "ERROR: " << use.what() << std::endl;
	exit(-1);
  }
  // initialize our graph instance
  //_g=graph;
}


void GraphBuilder::write_graphviz(std::ostream& os) {
  std::map<std::string,std::string> graph_attr, vertex_attr, edge_attr;
  graph_attr["size"] = "3,3";
  graph_attr["rankdir"] = "LR";
  graph_attr["ratio"] = "fill";
  vertex_attr["shape"] = "circle";

  boost::dynamic_properties dp;
  dp.property("label", boost::get(&vertex_t::name, graph));
  dp.property("node_id", boost::get(boost::vertex_index, graph));
  dp.property("label", boost::get(&edge_t::name, graph));

  boost::write_graphviz(os, graph, 
	  boost::make_label_writer(boost::get(&vertex_t::name, graph)), //_names[0]),
	boost::make_label_writer(boost::get(&edge_t::name, graph)), //_names[0]),
	boost::make_graph_attributes_writer(graph_attr, vertex_attr, edge_attr)
	  );
}


