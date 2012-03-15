#ifndef LIBHBA_GRAPH_BUILDER_HPP
#define LIBHBA_GRAPH_BUILDER_HPP 1

#include <libhba/common.hpp>
#include <libhba/ast_datatypes.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <iostream>
#include <string>

namespace hexabus {
  struct vertex_t{
	std::string name; 
  };

  struct edge_t{
	std::string name; 
  };

  typedef boost::adjacency_list<  // adjacency_list is a template depending on :
	boost::listS,               //  The container used for egdes : here, std::list.
	boost::vecS,                //  The container used for vertices: here, std::vector.
	//boost::directedS,           //  directed or undirected edges ?.
	boost::bidirectionalS,           //  directed or undirected edges ?.
	vertex_t,                     //  The type that describes a Vertex.
	edge_t                        //  The type that describes an Edge
  > graph_t; 
  typedef boost::graph_traits<graph_t>::vertex_descriptor vertex_id_t;
  typedef boost::graph_traits<graph_t>::edge_descriptor edge_id_t;

  class GraphBuilder {
	public:
	  typedef std::tr1::shared_ptr<GraphBuilder> Ptr;
	  GraphBuilder () : _g() {};
	  virtual ~GraphBuilder() {};

	  const graph_t get_graph() const { return _g; };
	  void write_graphviz(std::ostream& os);
	  void operator()(hba_doc const& hba) const;

	private:
	  GraphBuilder (const GraphBuilder& original);
	  GraphBuilder& operator= (const GraphBuilder& rhs);

	  void check_unreachable_states() const;

	  graph_t _g;

  };

};


#endif /* LIBHBA_GRAPH_BUILDER_HPP */
