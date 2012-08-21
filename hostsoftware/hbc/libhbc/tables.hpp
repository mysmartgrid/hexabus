#ifndef LIBHBC_TABLES_HPP
#define LIBHBC_TABLES_HPP

#include <string>
#include <vector>
#include <libhbc/common.hpp>
#include "libhbc/hbc_enums.hpp"

namespace hexabus {

  struct endpoint {
    unsigned int eid;
    datatype dtype;
    std::string name;
    bool read;
    bool write;
    bool broadcast;
  };

  struct device_alias {
    // TODO name, ip address, endpoint list
  };

  typedef std::vector<endpoint> endpoint_table;
  typedef std::tr1::shared_ptr<endpoint_table> endpoint_table_ptr;
};

#endif // LIBHBC_TABLES_HPP
