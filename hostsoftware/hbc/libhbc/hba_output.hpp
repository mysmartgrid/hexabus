#ifndef LIBHBC_HBA_OUTPUT_HPP
#define LIBHBC_HBA_OUTPUT_HPP

#include <libhbc/common.hpp>
#include <libhbc/graph.hpp>
#include <libhbc/tables.hpp>

namespace hexabus {
  class HBAOutput {
    public:
      typedef std::tr1::shared_ptr<HBAOutput> Ptr;
      HBAOutput(std::string device_name, graph_t_ptr g, device_table_ptr d, endpoint_table_ptr e, std::map<unsigned int, std::string> machine_filenames) : _dev_name(device_name), _g(g), _d(d), _e(e), machine_filenames_per_id(machine_filenames) {};
      virtual ~HBAOutput() {};

      void operator()(std::ostream& ostr);

    private:
      std::string _dev_name;
      graph_t_ptr _g;
      device_table_ptr _d;
      endpoint_table_ptr _e;
      std::map<unsigned int, std::string> machine_filenames_per_id;

      void print_condition(atomic_condition_doc at_cond, std::ostream& ostr, vertex_t& vertex);
      void print_condition(timeout_condition_doc to_cond, std::ostream& ostr, vertex_t& vertex);
      void print_condition(timer_condition_doc tm_cond, std::ostream& ostr, vertex_t& vertex);
      void print_ipv6address(boost::asio::ip::address_v6, std::ostream& ostr);
  };
};

#endif // LIBHBC_HBA_OUTPUT_HPP

