#ifndef LIBHBC_TABLE_BUILDER_HPP
#define LIBHBC_TABLE_BUILDER_HPP

#include "libhbc/tables.hpp"
#include "libhbc/ast_datatypes.hpp"

namespace hexabus {

  class TableBuilder {
    public:
      typedef std::tr1::shared_ptr<TableBuilder> Ptr;
      TableBuilder() {};
      virtual ~TableBuilder() {};

      endpoint_table_ptr get_table() const { return _e; };

      void operator()(hbc_doc& hbc);

    private:
      endpoint_table_ptr _e;
  };
};

#endif // LIBHBC_TABLE_BUILDER_HPP
