#ifndef LIBHBC_GRAPH_CHECKS
#define LIBHBC_GRAPH_CHECKS

#include <libhbc/common.hpp>
#include <libhbc/graph.hpp>
#include <libhbc/tables.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <iostream>

namespace hexabus {

  class GraphChecks {
    public:
      typedef std::tr1::shared_ptr<GraphChecks> Ptr;
      GraphChecks(graph_t_ptr g) : _g(g) {};
      virtual ~GraphChecks() {};

      void operator()();
      void find_states_without_inbound();
      void find_states_without_outgoing();
      void find_unreachable_states();
      void reachable_from_anywhere(std::string state_name, std::string machine_name);
      void never_reachable(std::string name, std::string machine_name);

    private:
      graph_t_ptr _g;
      template <typename T> bool exists(T elem, std::vector<T> vect);
  };

  // Visitor class for reachability analysis: Takes all nodes which it finds when handed to boost::breadth_first_search and puts them into a vector
  class bfs_nodelist_maker : public boost::default_bfs_visitor {
    public:
      bfs_nodelist_maker(std::vector<vertex_id_t>* node_ids) : node_id_list(node_ids) { }
      template <typename Vertex, typename Graph> void discover_vertex(Vertex u, Graph g) {
        if(!exists(u, *node_id_list)) {
          node_id_list->push_back(u);
        }
      }

    private:
      std::vector<vertex_id_t>* node_id_list;
      bool exists(vertex_id_t elem, std::vector<vertex_id_t> vect) {
        bool exists = false;
        BOOST_FOREACH(vertex_id_t the_elem, vect)
          if(the_elem == elem)
            exists = true;
        return exists;
      }
  };

  class bfs_predecessor_map_maker : public boost::default_bfs_visitor {
    public:
      bfs_predecessor_map_maker(std::map<vertex_id_t, vertex_id_t>* predecessor_map) : predecessors(predecessor_map) { }

      template <typename Edge, typename Graph> void tree_edge(Edge e, Graph g) {
        predecessors->insert(std::pair<vertex_id_t, vertex_id_t>(target(e,g), source(e,g)));
      }

    private:
      std::map<vertex_id_t, vertex_id_t>* predecessors;
  };
};

#endif // LIBHBC_GRAPH_CHECKS
