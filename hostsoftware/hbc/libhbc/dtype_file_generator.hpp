#ifndef DTYPE_FILE_GENERATOR_HPP
#define DTYPE_FILE_GENERATOR_HPP

#include <libhbc/common.hpp>
#include <libhbc/ast_datatypes.hpp>
#include <libhbc/tables.hpp>
#include <fstream>

namespace hexabus {

  class DTypeFileGenerator {
    public:
      typedef std::tr1::shared_ptr<DTypeFileGenerator> Ptr;
      DTypeFileGenerator() {};
      virtual ~DTypeFileGenerator() {};

      void operator()(endpoint_table_ptr ept, std::ofstream &ofs);

    private:
  };
};

#endif // DTYPE_FILE_GENERATOR_HPP

