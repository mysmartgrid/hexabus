#ifndef LIBHBC_GRAPH_TRANSFORMATION
#define LIBHBC_GRAPH_TRANSFORMATION

#include <libhbc/common.hpp>
#include <libhbc/graph.hpp>
#include <libhbc/tables.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <iostream>

namespace hexabus {
  class GraphTransformation {
    public:
      typedef std::tr1::shared_ptr<GraphTransformation> Ptr;
      GraphTransformation(device_table_ptr d, endpoint_table_ptr e) : _d(d), _e(e) {};
      virtual ~GraphTransformation() {};

      void operator()(graph_t_ptr in_g);

    private:
      device_table_ptr _d;
      endpoint_table_ptr _e;
      std::vector<vertex_id_t> slice_for_device(std::string dev_name, std::vector<vertex_t> vertices, graph_t_ptr g);
  };

  template <typename T> bool contains(std::vector<T>& v, T s) {
    BOOST_FOREACH(T t, v) {
      if(t == s)
        return true;
    }
    return false;
  }

  class bfs_nodelist_maker : public boost::default_bfs_visitor {
    public:
      bfs_nodelist_maker(std::vector<vertex_id_t>* node_ids) : node_id_list(node_ids) { }
      template <typename Vertex, typename Graph> void discover_vertex(Vertex u, Graph g) {
        // TODO there must be a more elegant way to do this
        if(!contains(*node_id_list, u))
          node_id_list->push_back(u);
      }

    private:
      std::vector<vertex_id_t>* node_id_list;
  };
}

#endif // LIBHBC_GRAPH_TRANSFORMATION
