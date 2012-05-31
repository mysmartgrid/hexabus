#include "graph_checks.hpp"
#include <sstream>

using namespace hexabus;

void GraphChecks::check_states_incoming() const {
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

void GraphChecks::check_states_outgoing() const {
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

void GraphChecks::check_conditions_incoming() const {
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

void GraphChecks::check_conditions_outgoing() const {
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
      // size_t num_follow_states=std::distance(inedgeIt, inedgeEnd);
      /* if (num_follow_states != 2) {
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
      } */
    }
  }
}



