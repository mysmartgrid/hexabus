#ifndef LIBHBA_GRAPH_HPP
#define LIBHBA_GRAPH_HPP 1

#include <libhba/common.hpp>
#include <libhba/ast_datatypes.hpp>
#include <boost/graph/adjacency_list.hpp>

namespace hexabus {
 enum vertex_type {CONDITION, STATE, STARTSTATE};
  struct vertex_t{
    std::string name; 
    unsigned int lineno;
    // this is a unique ID that is used to reference the vertex in the 
    // output stage.
    unsigned int id;
    vertex_type type;
  };

  enum edge_type {GOOD_STATE, BAD_STATE, FROM_STATE};
  struct edge_t{
    std::string name; 
    unsigned int lineno;
    edge_type type;
  };

  typedef boost::adjacency_list<  // adjacency_list is a template depending on :
    boost::vecS,               //  The container used for egdes : here, std::list.
    boost::vecS,                //  The container used for vertices: here, std::vector.
    //boost::directedS,           //  directed or undirected edges ?.
    boost::bidirectionalS,           //  directed or undirected edges ?.
    vertex_t,                     //  The type that describes a Vertex.
    edge_t                        //  The type that describes an Edge
      > graph_t; 
  typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_id_t;
  typedef boost::graph_traits<graph_t>::edge_descriptor edge_id_t;
  typedef std::tr1::shared_ptr<graph_t> graph_t_ptr;

 
};


#endif /* LIBHBA_GRAPH_HPP */

