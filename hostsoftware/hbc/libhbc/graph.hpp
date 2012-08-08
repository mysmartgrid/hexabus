#ifndef LIBHBC_GRAPH_HPP
#define LIBHBC_GRAPH_HPP

#include <libhbc/common.hpp>
#include <boost/graph/adjacency_list.hpp>

namespace hexabus {

  enum vertex_type { v_cond, v_state };
  struct vertex_t {
    std::string name;
    unsigned int machine_id;
    unsigned int state_id;    // state is identified by pair of machine and state id
    vertex_type type;
  };

  enum edge_type { e_edge }; // TODO do we have multiple types? Do we need this?
  struct edge_t {
    std::string name;
    unsigned int lineno;
    edge_type type;
  };

  typedef boost::adjacency_list<
    boost::vecS,        // Container used for edges
    boost::vecS,        // Container used for vertices
    boost::directedS,   // directed, undirected, bidirectional? TODO
    vertex_t,           // vertex type
    edge_t              // edge type
  > graph_t;

  typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_id_t;
  typedef boost::graph_traits<graph_t>::edge_descriptor   edge_id_t;
  typedef std::tr1::shared_ptr<graph_t> graph_t_ptr;

  // TODO
  // vertex_id_t find_vertex(graph_t_ptr g, const std::string& name);
  // unsigned int find_vertex_id(graph_t_ptr g, const std::string& name);
};

#endif // LIBHBC_GRAPH_HPP
