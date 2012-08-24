#include "graph.hpp"

#include <libhbc/error.hpp>

namespace hexabus {
  vertex_id_t find_vertex(graph_t_ptr g, unsigned int machine_id, unsigned int vertex_id) {
    graph_t::vertex_iterator vertexIt, vertexEnd;
    boost::tie(vertexIt, vertexEnd) = vertices((*g));
    for(; vertexIt != vertexEnd; vertexIt++) {
      vertex_id_t vertexID = *vertexIt;
      vertex_t& vertex = (*g)[vertexID];
      if(vertex.machine_id == machine_id && vertex.vertex_id == vertex_id)
        return vertexID;
    }
    // not found
    std::ostringstream oss;
    oss << "Machine ID: " << machine_id << ", Vertex ID: " << vertex_id;
    throw VertexNotFoundException(oss.str());
  }

  unsigned int find_state_vertex_id(graph_t_ptr g, statemachine_doc statemachine, const std::string name) {
    bool found = false;
    unsigned int state_id;
    for(unsigned int i = 0; i < statemachine.stateset.states.size() && !found; i++) {
      if(name == statemachine.stateset.states[i]) {
        found = true;
        state_id = i;
      }
    }
    if(!found) {
      std::ostringstream oss;
      oss << "State name not found: " << statemachine.name << "." << name;
      throw StateNameNotFoundException(oss.str());
    }
    return state_id;
  }
};
