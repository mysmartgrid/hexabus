#ifndef LIBHBC_HBA_OUTPUT_HPP
#define LIBHBC_HBA_OUTPUT_HPP

#include <libhbc/common.hpp>
#include <libhbc/graph.hpp>
#include <libhbc/tables.hpp>

namespace hexabus {
  class HBAOutput {
    public:
      typedef std::tr1::shared_ptr<HBAOutput> Ptr;
      HBAOutput(graph_t_ptr g, device_table_ptr d, endpoint_table_ptr e) : _g(g), _d(d), _e(e) {};
      virtual ~HBAOutput() {};

      void operator()(std::ostream& ostr);

    private:
      graph_t_ptr _g;
      device_table_ptr _d;
      endpoint_table_ptr _e;
  };
}

#endif // LIBHBC_HBA_OUTPUT_HPP

