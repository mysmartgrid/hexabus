#ifndef LIBHBC_MODULE_INSTANTIATION_HPP
#define LIBHBC_MODULE_INSTANTIATION_HPP

#include "libhbc/tables.hpp"
#include "libhbc/ast_datatypes.hpp"

namespace hexabus {

  class ModuleInstantiation {
    public:
      typedef std::tr1::shared_ptr<ModuleInstantiation>  Ptr;
      ModuleInstantiation() : _m(new module_table()) {};
      virtual ~ModuleInstantiation() {};

      void operator()(hbc_doc& hbc);
      void print_module_table();

    private:
      module_table_ptr _m;
  };
}

#endif // LIBHBC_MODULE_INSTANTIATION_HPP
