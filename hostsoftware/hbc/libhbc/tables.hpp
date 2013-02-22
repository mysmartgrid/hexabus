#ifndef LIBHBC_TABLES_HPP
#define LIBHBC_TABLES_HPP

#include <string>
#include <map>
#include <boost/asio.hpp>
#include <libhbc/common.hpp>
#include "libhbc/hbc_enums.hpp"
#include "libhbc/ast_datatypes.hpp"

namespace hexabus {

  struct endpoint {
    unsigned int eid;
    datatype dtype;
    bool read;
    bool write;
    bool broadcast;
  };

  struct device_alias {
    boost::asio::ip::address_v6 ipv6_address;
    std::vector<unsigned int> eids;
  };

	struct machine_descriptor {
		std::string name; // the name of the state machine (as given in the source code)
		std::string file; // the name of the file the machine was read from
	};

  typedef std::map<std::string, device_alias> device_table;
  typedef std::map<std::string, endpoint> endpoint_table;
  typedef std::map<std::string, module_doc> module_table;
	typedef std::map<unsigned int, machine_descriptor> machine_table;
  typedef std::tr1::shared_ptr<device_table> device_table_ptr;
  typedef std::tr1::shared_ptr<endpoint_table> endpoint_table_ptr;
  typedef std::tr1::shared_ptr<module_table> module_table_ptr;
};

#endif // LIBHBC_TABLES_HPP
