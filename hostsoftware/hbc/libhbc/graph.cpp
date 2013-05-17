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
    unsigned int state_id = 0;
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

  vertex_id_t add_vertex(graph_t_ptr g, std::string name, unsigned int machine_id, unsigned int vertex_id, vertex_type type, boost::variant<condition_doc, command_block_doc> contents) {
    // remove linebreaks from name
    size_t found = name.find("\n");
    while(found != std::string::npos) {
      name.replace(found, 1, "\\n");
      found = name.find("\n");
    }

    // add vertex
    vertex_id_t v_id = boost::add_vertex((*g));

    // fill structure with values
    (*g)[v_id].name = name;
    (*g)[v_id].machine_id = machine_id;
    (*g)[v_id].vertex_id = vertex_id;
    (*g)[v_id].type = type;
    (*g)[v_id].contents = contents;

    return v_id;
  }

  vertex_id_t add_vertex(graph_t_ptr g, std::string name, unsigned int machine_id, unsigned int vertex_id, vertex_type type) {
    // remove linebreaks from name
    replaceNewline(name);

    // add vertex
    vertex_id_t v_id = boost::add_vertex((*g));

    // fill structure with values
    (*g)[v_id].name = name;
    (*g)[v_id].machine_id = machine_id;
    (*g)[v_id].vertex_id = vertex_id;
    (*g)[v_id].type = type;

    return v_id;
  }

  edge_id_t add_edge(graph_t_ptr g, vertex_id_t from, vertex_id_t to, edge_type type) {
    edge_id_t edge;
    bool ok;
    boost::tie(edge, ok) = boost::add_edge(from, to, (*g));
    if(ok)
      (*g)[edge].type = type;
    else {
      std::ostringstream oss;
      oss << "Cannot link vertex " << (*g)[from].machine_id << "." << (*g)[from].vertex_id << " to vertex " << (*g)[to].machine_id << "." << (*g)[to].vertex_id << std::endl;
      throw EdgeLinkException(oss.str());
    }

    return edge;
  }

  void replaceNewline(std::string& str) {
    size_t found = str.find("\n");
    while(found != std::string::npos) {
      str.replace(found, 1, "\\n");
      found = str.find("\n");
    }
  }

};

