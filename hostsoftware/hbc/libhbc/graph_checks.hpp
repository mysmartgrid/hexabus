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
      void find_unreachable_states();

    private:
      graph_t_ptr _g;
  };
};

#endif // LIBHBC_GRAPH_CHECKS
