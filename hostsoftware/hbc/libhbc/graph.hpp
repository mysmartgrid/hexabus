#ifndef LIBHBC_GRAPH_HPP
#define LIBHBC_GRAPH_HPP

#include <libhbc/common.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <libhbc/ast_datatypes.hpp>

namespace hexabus {

  enum vertex_type { v_cond, v_state, v_command };
  struct vertex_t {
    std::string name;
    unsigned int machine_id;
    unsigned int vertex_id;    // state is identified by pair of machine and state id
    vertex_type type;
    boost::variant<condition_doc, command_block_doc> contents;
    boost::default_color_type color; // needs to be colorable for breadth first search
  };

  enum edge_type { e_from_state, e_if_com, e_to_state, e_transformed };
  struct edge_t {
    std::string name;
    unsigned int lineno;
    edge_type type;
  };

  typedef boost::adjacency_list<
    boost::vecS,        // Container used for edges
    boost::vecS,        // Container used for vertices
    boost::directedS,   // directed edges.
    vertex_t,           // vertex type
    edge_t              // edge type
  > graph_t;

  typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_id_t;
  typedef boost::graph_traits<graph_t>::edge_descriptor edge_id_t;
  typedef std::tr1::shared_ptr<graph_t> graph_t_ptr;

  vertex_id_t find_vertex(graph_t_ptr g, unsigned int machine_id, unsigned int vertex_id);
  unsigned int find_state_vertex_id(graph_t_ptr g, statemachine_doc statemachine, const std::string name);
  vertex_id_t add_vertex(graph_t_ptr g, std::string name, unsigned int machine_id, unsigned int vertex_id, vertex_type type, boost::variant<condition_doc, command_block_doc> contents);
  vertex_id_t add_vertex(graph_t_ptr g, std::string name, unsigned int machine_id, unsigned int vertex_id, vertex_type type);
  edge_id_t add_edge(graph_t_ptr g, vertex_id_t from, vertex_id_t to, edge_type type);

  void replaceNewline(std::string& str);
};

#endif // LIBHBC_GRAPH_HPP
