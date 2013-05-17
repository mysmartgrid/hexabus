#ifndef LIBHBC_TABLE_BUILDER_HPP
#define LIBHBC_TABLE_BUILDER_HPP

#include "libhbc/tables.hpp"
#include "libhbc/ast_datatypes.hpp"

namespace hexabus {

  class TableBuilder {
    public:
      typedef std::tr1::shared_ptr<TableBuilder> Ptr;
      TableBuilder() : _e(new endpoint_table()), _d(new device_table()), _m(new module_table()) {};
      virtual ~TableBuilder() {};

      endpoint_table_ptr get_endpoint_table() const { return _e; };
      device_table_ptr get_device_table() const { return _d; };
      module_table_ptr get_module_table() const { return _m; };

      void operator()(hbc_doc& hbc);
      void print();

    private:
      endpoint_table_ptr _e;
      device_table_ptr _d;
      module_table_ptr _m;
  };
};

#endif // LIBHBC_TABLE_BUILDER_HPP
