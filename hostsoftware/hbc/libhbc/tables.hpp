#ifndef LIBHBC_TABLES_HPP
#define LIBHBC_TABLES_HPP

#include <string>
#include <map>
#include <boost/asio.hpp>
#include <libhbc/common.hpp>
#include "libhbc/hbc_enums.hpp"

namespace hexabus {

  struct endpoint {
    datatype dtype;
    std::string name;
    bool read;
    bool write;
    bool broadcast;
  };

  struct device_alias {
    boost::asio::ip::address ipv6_address;
    std::vector<unsigned int> eids;
  };

  typedef std::map<std::string, device_alias> device_table;;
  typedef std::map<unsigned int, endpoint> endpoint_table;
  typedef std::tr1::shared_ptr<device_table> device_table_ptr;
  typedef std::tr1::shared_ptr<endpoint_table> endpoint_table_ptr;
};

#endif // LIBHBC_TABLES_HPP
