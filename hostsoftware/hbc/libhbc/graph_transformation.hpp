#ifndef LIBHBC_GRAPH_TRANSFORMATION
#define LIBHBC_GRAPH_TRANSFORMATION

#include <libhbc/common.hpp>
#include <libhbc/graph.hpp>
#include <libhbc/tables.hpp>

namespace hexabus {
  class GraphTransformation {
    public:
      typedef std::tr1::shared_ptr<GraphTransformation> Ptr;
      GraphTransformation(device_table_ptr d, endpoint_table_ptr e/* TODO */) : _d(d), _e(e) {};
      virtual ~GraphTransformation() {};

      void operator()(graph_t_ptr in_g);

    private:
      device_table_ptr _d;
      endpoint_table_ptr _e;
      bool contains(std::vector<std::string> v, std::string s);
  };
}

#endif // LIBHBC_GRAPH_TRANSFORMATION
