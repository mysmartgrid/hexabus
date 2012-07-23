#ifndef LIBHBC_AST_DATATYPES
#define LIBHBC_AST_DATATYPES

#include <libhbc/common.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>

#include <boost/variant/recursive_variant.hpp>
#include <boost/spirit/home/qi.hpp> 
#include <boost/spirit/home/support/info.hpp> 
#include <boost/spirit/home/phoenix.hpp> 
#include <boost/spirit/home/phoenix/statement/sequence.hpp> 

namespace hexabus {

// Data structures that hold the syntax tree
  struct include_doc {
    unsigned int lineno;
    std::string filename;
  };

  struct eid_list_doc {
    std::vector<unsigned int> eids;
  };

  struct alias_doc {
    unsigned int lineno;
    std::string device_name;
    std::string ipv6_address;
    eid_list_doc eid_list;
  };

  struct global_endpoint_id_doc {
    std::string device_alias;
    std::string endpoint_name;
  };

  struct condition_doc {
    global_endpoint_id_doc geid;
    // TODO comparison operator, once there are more operators
    float constant;
  };

  struct write_command_doc {
    global_endpoint_id_doc geid;
    unsigned int lineno;
    float constant;
  };

  struct command_doc {
    write_command_doc write_command; // TODO boost::variant if there's more
  };

  struct goto_command_doc {
    unsigned int lineno;
    std::string target_state;
  };

 struct if_clause_doc {
    unsigned int lineno;
    condition_doc condition;
    std::vector<command_doc> commands;
    goto_command_doc goto_command;
  };

  struct in_clause_doc {
    unsigned int lineno;
    // std::vector<if_clause_doc> if_clauses;
  };

  struct stateset_doc {
    std::vector<std::string> states;
  };

  struct statemachine_doc {
    std::string name;
    stateset_doc stateset;
    std::vector<in_clause_doc> in_clauses;
  };

  typedef boost::variant<include_doc, alias_doc, statemachine_doc> hbc_block;

  struct hbc_doc {
    std::vector<hbc_block> blocks;
  };
};

// how are the structures filled with data
BOOST_FUSION_ADAPT_STRUCT(
  hexabus::include_doc,
  (unsigned int, lineno)
  (std::string, filename)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::eid_list_doc,
  (std::vector<unsigned int>, eids)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::alias_doc,
  (unsigned int, lineno)
  (std::string, device_name)
  (std::string, ipv6_address)
  (hexabus::eid_list_doc, eid_list)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::global_endpoint_id_doc,
  // (std::string, device_alias) TODO
  // (std::string, endpoint_name)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::statemachine_doc,
  (std::string, name)
  (hexabus::stateset_doc, stateset)
  (std::vector<hexabus::in_clause_doc>, in_clauses)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::in_clause_doc,
  (unsigned int, lineno)
  // (std::vector<hexabus::if_clause_doc>, if_clauses) TODO
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::stateset_doc,
  (std::vector<std::string>, states)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::if_clause_doc,
  (unsigned int, lineno)
  (hexabus::condition_doc, condition)
  (std::vector<hexabus::command_doc>, commands)
  (hexabus::goto_command_doc, goto_command)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::command_doc,
  (hexabus::write_command_doc, write_command)   // TODO variant if there's more
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::condition_doc,
  (hexabus::global_endpoint_id_doc, geid)
  (float, constant)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::goto_command_doc,
  (unsigned int, lineno)
  (std::string, target_state)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::write_command_doc,
  (hexabus::global_endpoint_id_doc, geid)
  (unsigned int, lineno)
  (float, constant)
)

BOOST_FUSION_ADAPT_STRUCT(
  hexabus::hbc_doc,
  (std::vector<hexabus::hbc_block>, blocks)
)

#endif // LIBHBC_AST_DATATYPES

