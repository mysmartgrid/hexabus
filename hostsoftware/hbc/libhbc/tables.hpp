#ifndef LIBHBC_TABLES_HPP
#define LIBHBC_TABLES_HPP

#include <string>
#include <vector>
#include <libhbc/common.hpp>
#include "libhbc/hbc_enums.hpp"

namespace hexabus {

  // TODO maybe this should be a map eid->endpoint struct
  struct endpoint {
    unsigned int eid;
    datatype dtype;
    std::string name;
    bool read;
    bool write;
    bool broadcast;
  };

  // TODO maybe this shoult be a map name->device struct
  struct device_alias {
    std::string name;
    std::string ipv6_address; // TODO Parse it. Before you put it here.
    std::vector<unsigned int> eids;
  };

  typedef std::vector<device_alias> device_table;
  typedef std::vector<endpoint> endpoint_table;
  typedef std::tr1::shared_ptr<device_table> device_table_ptr;
  typedef std::tr1::shared_ptr<endpoint_table> endpoint_table_ptr;
};

#endif // LIBHBC_TABLES_HPP
