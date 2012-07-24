#ifndef LIBHBC_HBCOMP_GRAMMAR
#define LIBHBC_HBCOMP_GRAMMAR

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/home/phoenix.hpp>
#include <boost/spirit/home/phoenix/statement/sequence.hpp>
#include <boost/spirit/home/qi.hpp>
#include <boost/spirit/home/support/info.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/variant/recursive_variant.hpp>

#include "../../../shared/hexabus_statemachine_structs.h"

#include <libhbc/file_pos.hpp>

namespace qi = boost::spirit::qi;
namespace classic = boost::spirit::classic;

typedef std::istreambuf_iterator<char> base_iterator_type;
typedef boost::spirit::multi_pass<base_iterator_type> forward_iterator_type;
typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;

namespace hexabus {

  struct error_traceback_t
  {
    error_traceback_t (std::string const & msg)
    {
      stack.push_back(msg);
    }

    template <typename ArgsT, typename ContextT, typename RT>
      void operator() (ArgsT, ContextT, RT) const
    { }

    static std::vector<std::string> stack;
  };
  std::vector<std::string> error_traceback_t::stack;

  struct FileState
  {
    FileState() : line(1)
    { }

    int line;
  };

  struct UpdateFileInfo
  {
    UpdateFileInfo(FileState& state) : m_state(state)
    {}

    template <typename Iterator, typename ArgT1, typename ArgT2, typename ArgT3>
      void operator() (Iterator begin, Iterator end, ArgT1 a1, ArgT2 a2, ArgT3 a3) const
    {
      // TODO
    }

    template <typename ArgT1, typename ArgT2, typename ArgT3>
      void operator() (ArgT1 a1, ArgT2 a2, ArgT3 a3) const
    {
      // TODO
    }

    template <typename Iterator>
      void operator() (Iterator begin, Iterator end) const
    {
      // TODO
    }

    FileState& m_state;
  };

  // Grammar definition
  template <typename Iterator>
    struct hexabus_comp_grammar : public qi::grammar<Iterator, hbc_doc(), skipper<Iterator> >
  {
    typedef skipper<Iterator> Skip;

    hexabus_comp_grammar(std::vector<std::string>& error_hints,
      pos_iterator_type& current_position) : hexabus_comp_grammar::base_type(start),
        _error_hints(error_hints), _current_position(current_position)
    {
      using qi::lit;
      using qi::lexeme;
      using qi::on_error;
      using qi::rethrow;
      using qi::fail;
      using qi::accept;
      using boost::spirit::ascii::char_;
      using qi::uint_;
      using qi::float_;
      using boost::spirit::ascii::string;
      using boost::spirit::ascii::space;
      using namespace qi::labels;
      using boost::spirit::_1;
      using boost::spirit::_2;
      using boost::spirit::_3;
      using boost::spirit::_4;
      using boost::spirit::eps;
      using boost::spirit::eoi;
      using boost::spirit::repository::qi::file_pos;

      using boost::phoenix::construct;
      using boost::phoenix::val;
      using boost::phoenix::ref;
      using boost::phoenix::bind;
      using boost::phoenix::let;

      // Assignment and comparison operators, constants, ...
      is = eps > lit(":=");
      equals = lit("==") [_val = 0]; // TODO use STM_EQ and others from global define
      constant = float_; // TODO do we need to distinguish between float, int, timestamp, ...?

      // Basic elements: Identifier, assignment, ...
      identifier %= eps
        > char_("a-zA-Y_") > *char_("a-zA-Z0-9_");
      on_error<rethrow>(identifier, error_traceback_t("Invalid identifier"));
      // device_name.endpoint_name
      global_endpoint_id %= identifier > '.' > identifier;
      filename = identifier; // TODO most probably we need to be more specific here
      // TODO Accept shortened IPv6 addresses - check validity with boost's network functionality
      ipv6_address_block = char_("0-9a-fA-F") > char_("0-9a-fA-F") > char_("0-9a-fA-F") > char_("0-9a-fA-F");
      ipv6_address = ((ipv6_address_block [_val+=_1]) > ':' > (ipv6_address_block [_val+=_1]) > ':' >
                     (ipv6_address_block [_val+=_1]) > ':' > (ipv6_address_block [_val+=_1]) > ':' >
                     (ipv6_address_block [_val+=_1]) > ':' > (ipv6_address_block [_val+=_1]) > ':' >
                     (ipv6_address_block [_val+=_1]) > ':' > (ipv6_address_block [_val+=_1]));

      datatype = ( lit("BOOL") | lit("UINT8") | lit("UINT32") | lit("FLOAT") );
      access_level = ( lit("read") | lit("write") | lit("broadcast") );

      // larger blocks: Aliases, state machines
      include %= lit("include") >> file_pos > filename > ';';
      endpoint %= lit("endpoint") >> file_pos > uint_ > '{' > lit("datatype") > datatype > ';'
        > lit("name") > identifier > ';' > lit("access") > *access_level > ';' > '}'; // TODO allow datatype, name, access in any order
      eid_list %= '{' > uint_ > *(',' > uint_) > '}';
      alias %= lit("alias") >> file_pos > identifier > '{'
        > lit("ip") > ipv6_address > ';'
        > lit("eids") > eid_list > ';' > '}';

      stateset %= lit("states") > '{' > identifier > *(',' > identifier) > '}';
      statemachine %= lit("machine") > identifier > '{' > stateset > ';' > *in_clause > '}';

      in_clause %= lit("in") >> file_pos > '(' > identifier > ')'
        > '{' > *if_clause > '}';

      condition %= global_endpoint_id > equals > constant; // TODO more comparison operators

      if_clause %= lit("if") >> file_pos > '(' > condition > ')'
        > '{' > *command > goto_command > ';' > '}';

      // commands that "do something"
      command %= write_command > ';'; // TODO more commands to be added here?
      write_command %= lit("write") > global_endpoint_id >> file_pos > is > constant;
      goto_command %= lit("goto") >> file_pos > identifier;

      start %= *( include | endpoint | alias | statemachine ) > eoi;

      start.name("toplevel");
      identifier.name("identifier");
      filename.name("filename");
      alias.name("alias");
      statemachine.name("statemachine");
      condition.name("condition");
      command.name("command");
      goto_command.name("goto command");

      on_error<rethrow> (start, error_traceback_t("Cannot parse input"));
    }

    qi::rule<Iterator, std::string(), Skip> startstate;
    qi::rule<Iterator, void(), Skip> is;
    qi::rule<Iterator, int(), Skip> equals;
    qi::rule<Iterator, float(), Skip> constant;
    qi::rule<Iterator, std::string(), Skip> identifier;
    qi::rule<Iterator, std::string(), Skip> filename;
    qi::rule<Iterator, datatype_doc(), Skip> datatype;
    qi::rule<Iterator, access_level_doc(), Skip> access_level;
    qi::rule<Iterator, endpoint_doc(), Skip> endpoint;
    qi::rule<Iterator, global_endpoint_id_doc(), Skip> global_endpoint_id;
    qi::rule<Iterator, std::string(), Skip> ipv6_address;
    qi::rule<Iterator, std::string(), Skip> ipv6_address_block;
    qi::rule<Iterator, include_doc(), Skip> include;
    qi::rule<Iterator, eid_list_doc(), Skip> eid_list;
    qi::rule<Iterator, alias_doc(), Skip> alias;
    qi::rule<Iterator, stateset_doc(), Skip> stateset;
    qi::rule<Iterator, statemachine_doc(), Skip> statemachine;
    qi::rule<Iterator, in_clause_doc(), Skip> in_clause;
    qi::rule<Iterator, condition_doc(), Skip> condition;
    qi::rule<Iterator, if_clause_doc(), Skip> if_clause;
    qi::rule<Iterator, command_doc(), Skip> command;
    qi::rule<Iterator, write_command_doc(), Skip> write_command;
    qi::rule<Iterator, goto_command_doc(), Skip> goto_command;
    qi::rule<Iterator, hbc_doc(), Skip> start;

    std::vector<std::string>& _error_hints;
    pos_iterator_type& _current_position;

    FileState file_state;
  };
};

#endif // LIBHBC_HBCOMP_GRAMMAR

