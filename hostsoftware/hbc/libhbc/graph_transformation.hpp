#ifndef LIBHBC_GRAPH_TRANSFORMATION
#define LIBHBC_GRAPH_TRANSFORMATION

namespace hexabus {
  class GraphTransformation {
    public:
      typedef std::tr1::shared_ptr<GraphTransformation> Ptr;
      GraphTransformation(graph_t_ptr in_g, device_table_ptr d, endpoint_table_ptr e/* TODO */) : _in_g(in_g), _d(d), _e(e) {};
      virtual ~GraphTransformation() {};

    private:
      graph_t_ptr _in_g;
      device_table_ptr _d;
      endpoint_table_ptr _e;
      std::map<std::string, graph_t_ptr> out_graphs;
  };
}

#endif // LIBHBC_GRAPH_TRANSFORMATION
