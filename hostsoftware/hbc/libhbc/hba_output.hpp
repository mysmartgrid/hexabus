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

      void print_condition(atomic_condition_doc at_cond, std::ostream& ostr, vertex_t& vertex);
      void print_condition(timeout_condition_doc to_cond, std::ostream& ostr, vertex_t& vertex);
      void print_condition(timer_condition_doc tm_cond, std::ostream& ostr, vertex_t& vertex);
  };
}

#endif // LIBHBC_HBA_OUTPUT_HPP

