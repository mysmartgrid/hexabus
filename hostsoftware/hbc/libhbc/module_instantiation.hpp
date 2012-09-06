#ifndef LIBHBC_MODULE_INSTANTIATION_HPP
#define LIBHBC_MODULE_INSTANTIATION_HPP

#include "libhbc/tables.hpp"
#include "libhbc/ast_datatypes.hpp"

namespace hexabus {

  class ModuleInstantiation {
    public:
      typedef std::tr1::shared_ptr<ModuleInstantiation>  Ptr;
      ModuleInstantiation(device_table_ptr devt, endpoint_table_ptr ept) : _m(new module_table()), _d(devt), _e(ept) {};
      virtual ~ModuleInstantiation() {};

      void operator()(hbc_doc& hbc);
      void print_module_table();

    private:
      module_table_ptr _m;
      device_table_ptr _d;
      endpoint_table_ptr _e;
  };
}

#endif // LIBHBC_MODULE_INSTANTIATION_HPP
