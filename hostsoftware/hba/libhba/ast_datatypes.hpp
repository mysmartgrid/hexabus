#ifndef LIBHBA_AST_DATATYPES_HPP
#define LIBHBA_AST_DATATYPES_HPP 1

#include <libhba/common.hpp>
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
  namespace fusion = boost::fusion;
  namespace phoenix = boost::phoenix;
  namespace ascii = boost::spirit::ascii;

  ///////////////////////////////////////////////////////////////////////////
  //  HexaBus assembler representation
  ///////////////////////////////////////////////////////////////////////////
  struct if_clause_doc {
    unsigned int lineno;
    std::string name;
    unsigned int eid;
    float value;
    std::string goodstate;
    std::string badstate;
  };

  struct state_doc {
    unsigned int lineno;
    std::string name;
    std::vector<if_clause_doc> if_clauses;
    unsigned int id;
  };

  struct cond_eidvalue_doc {
    unsigned int lineno;
    std::string ipv6_address;
    unsigned int eid;
    unsigned int op;
    float value;
  };

  struct cond_timeout_doc {
    unsigned int lineno;
    unsigned int value;
  };

  struct cond_datetime_doc {
    unsigned int lineno;
    unsigned int field;
    unsigned int op;
    unsigned int value;
  };

  typedef boost::variant<cond_eidvalue_doc, cond_timeout_doc, cond_datetime_doc> hba_cond;

  struct condition_doc {
    unsigned int lineno;
    std::string name;
    hba_cond cond;
    unsigned int id;
  };

  typedef
    boost::variant<
    state_doc              // one state definition
    , condition_doc          // one condition definition
    >
    hba_doc_block;

  struct hba_doc
  {
    std::string target_ip;
    std::string machine_id;
    std::string start_state;
    std::vector<hba_doc_block> blocks;
  };
};

// We need to tell fusion about our hba struct
// to make it a first-class fusion citizen

BOOST_FUSION_ADAPT_STRUCT(
    hexabus::if_clause_doc,
    (unsigned int, lineno)
    (std::string, name)
    (unsigned int, eid)
    (float, value)
    (std::string, goodstate)
    (std::string, badstate)
    )

BOOST_FUSION_ADAPT_STRUCT(
    hexabus::state_doc,
    (unsigned int, lineno)
    (std::string, name)
    (std::vector<hexabus::if_clause_doc>, if_clauses)
    )

BOOST_FUSION_ADAPT_STRUCT(
    hexabus::condition_doc,
    (unsigned int, lineno)
    (std::string, name)
    (hexabus::hba_cond, cond)
    )

BOOST_FUSION_ADAPT_STRUCT(
    hexabus::cond_eidvalue_doc,
    (std::string, ipv6_address)
    (unsigned int, eid)
    (unsigned int, op)
    (float, value)
    )

BOOST_FUSION_ADAPT_STRUCT(
    hexabus::cond_timeout_doc,
    (unsigned int, value)
    )

BOOST_FUSION_ADAPT_STRUCT(
    hexabus::cond_datetime_doc,
    (unsigned int, field)
    (unsigned int, op)
    (unsigned int, value)
    )

BOOST_FUSION_ADAPT_STRUCT(
    hexabus::hba_doc,
    (std::string, target_ip)
    (std::string, machine_id)
    (std::string, start_state)
    (std::vector<hexabus::hba_doc_block>, blocks)
    )


#endif /* LIBHBA_AST_DATATYPES_HPP */

