#include "graph.hpp"

namespace hexabus {
  vertex_id_t find_vertex(graph_t_ptr g, const std::string& name) {
    graph_t::vertex_iterator vertexIt, vertexEnd;
    boost::tie(vertexIt, vertexEnd) = vertices((*g));
    for (; vertexIt != vertexEnd; ++vertexIt){
      vertex_id_t vertexID = *vertexIt; // dereference vertexIt, get the ID
      vertex_t & vertex = (*g)[vertexID];
      if (name == vertex.name) {
        return vertexID;
      }
    }
    // we have not found an vertex id.
    std::ostringstream oss;
    oss << "cannot find vertex " << name;
    throw VertexNotFoundException(oss.str());
  }

  unsigned int find_vertex_id(graph_t_ptr g, const std::string& name) {
    vertex_id_t vertexID=find_vertex(g, name);
    vertex_t & vertex = (*g)[vertexID];
    return vertex.id;
  }
}
