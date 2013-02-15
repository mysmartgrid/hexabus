#ifndef LIBHBC_GRAPH_TRANSFORMATION
#define LIBHBC_GRAPH_TRANSFORMATION

#include <libhbc/common.hpp>
#include <libhbc/graph.hpp>
#include <libhbc/tables.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/visitors.hpp>
#include <iostream>

namespace hexabus {

  typedef std::map<unsigned int, std::vector<vertex_id_t> > machine_map_t;
  typedef std::pair<unsigned int, std::vector<vertex_id_t> > machine_map_elem;
  typedef std::multimap<std::string, std::vector<vertex_id_t> > device_map_t;

  class GraphTransformation {
    public:
      typedef std::tr1::shared_ptr<GraphTransformation> Ptr;
      GraphTransformation(device_table_ptr d, endpoint_table_ptr e, machine_table& machines) : _d(d), _e(e), _machines(machines) {};
      virtual ~GraphTransformation() {};
      void writeGraphviz(std::string filename_prefix);
      std::map<std::string, graph_t_ptr> getDeviceGraphs();

      void operator()(graph_t_ptr in_g);

    private:
      device_table_ptr _d;
      endpoint_table_ptr _e;
      std::map<std::string, graph_t_ptr> device_graphs;
      machine_table _machines;
      machine_map_t generateMachineMap(graph_t_ptr g);
      std::vector<std::string> findDevices(std::vector<vertex_id_t> stm_vertices, graph_t_ptr g);
      graph_t_ptr reconstructGraph(std::vector<vertex_id_t> vertices, graph_t_ptr in_g);
      template <typename T> bool exists(T elem, std::vector<T> vect);
  };
};

#endif // LIBHBC_GRAPH_TRANSFORMATION
