#ifndef LIBHBA_HBASM_GRAMMAR_HPP
#define LIBHBA_HBASM_GRAMMAR_HPP 1

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
#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>

// File position helper
//#include <boost/spirit/repository/home/qi/primitive/iter_pos.hpp>
#include <libhba/file_pos.hpp>

// We need this for the hexabus datatype definitions
#include "../../../shared/hexabus_definitions.h"
#include "../../../shared/hexabus_statemachine_structs.h"


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
      void operator () (ArgsT, ContextT, RT) const
      {
      }

    static std::vector<std::string> stack;
  };
  std::vector<std::string> error_traceback_t::stack;

struct FileState
{
  FileState()
  : line (1)
  {}

  int line;
};

struct UpdateFileInfo
{
  UpdateFileInfo (FileState & state)
  : m_state (state)
  {}

  template <typename Iterator, typename ArgT1, typename ArgT2, typename ArgT3>
  void operator () (Iterator begin, Iterator end, ArgT1 a1, ArgT2 a2, ArgT3 a3) const
  {
    m_state.line++;
    std::cout << "updatefileinfo(begin, end, a1, a2, a3): " << std::endl
        << "   " << typeid(begin).name() << std::endl
        << "   " << typeid(end).name() << std::endl
        << "   " << typeid(a1).name() << std::endl
        << "   " << typeid(a2).name() << std::endl
        << "   " << typeid(a3).name() << std::endl
       ;
  }


  template <typename ArgT1, typename ArgT2, typename ArgT3>
  void operator () (ArgT1 a1, ArgT2 a2, ArgT3 a3) const
  {
    m_state.line++;
    std::cout << "updatefileinfo(a1, a2, a3): " << std::endl
        << "   " << typeid(a1).name() << std::endl
        << "   " << typeid(a2).name() << std::endl
        << "   " << typeid(a3).name() << std::endl
       ;
  }

  template <typename IteratorType>
  void operator () (IteratorType begin, IteratorType end) const
  {
    m_state.line++;
    std::cout << "updatefileinfo(b, e): " << std::endl
          << "   " << typeid(begin).name() << std::endl
          << "   " << typeid(end).name() << std::endl;
  }

  FileState & m_state;
};

  ///////////////////////////////////////////////////////////////////////////
  //  Our grammar definition
  ///////////////////////////////////////////////////////////////////////////
  template <typename Iterator>
  struct hexabus_asm_grammar
  : public qi::grammar<Iterator, hba_doc(), skipper<Iterator> >
  {
    typedef skipper<Iterator> Skip;

    hexabus_asm_grammar(
      std::vector<std::string>& error_hints,
      pos_iterator_type& current_position
    ) : hexabus_asm_grammar::base_type(start),
      _error_hints(error_hints),
      _current_position(current_position)
    {
    using qi::lit;
    using qi::lexeme;
    using qi::on_error;
    using qi::rethrow;
    using qi::fail;
    using qi::accept;
    using ascii::char_;
    using qi::uint_;
    using qi::float_;
    using ascii::string;
    using ascii::space;
    using namespace qi::labels;
    using boost::spirit::_1;
    using boost::spirit::_2;
    using boost::spirit::_3;
    using boost::spirit::_4;
    using boost::spirit::eps;
    using boost::spirit::eoi;
    using boost::spirit::repository::qi::file_pos;

    using phoenix::construct;
    using phoenix::val;
    using phoenix::ref;
    using phoenix::bind;
    using phoenix::let;

    /************************************************************
     * CAVEAT!
     * >> is the sequence operator. a >> b means that a followed
     * by b is parsed. Don't confuse with
     * >, which is the expect operator. a > b means that a
     * *must* be followed by b, otherwise an expectation_failure
     * is thrown (which triggers the error handling attached to
     * the rule.)
     ***********************************************************/

    is = eps > lit(":=");
    equals = lit("==") [_val = STM_EQ];
    lessequal = lit("<=") [_val = STM_LEQ];
    greaterequal = lit(">=") [_val = STM_GEQ];
    lessthan = lit("<") [_val = STM_LT];
    greaterthan = lit(">") [_val = STM_GT];
    notequal = lit("!=") [_val = STM_NEQ];
    eid_value = (uint_ [_val=_1]);
    hour = lit("hour") [_val = HXB_SM_HOUR];
    minute = lit("minute") [_val = HXB_SM_MINUTE];
    second = lit("second") [_val = HXB_SM_SECOND];
    day = lit("day") [_val = HXB_SM_DAY];
    month = lit("month") [_val = HXB_SM_MONTH];
    year = lit("year") [_val = HXB_SM_YEAR];
    weekday = lit("weekday") [_val = HXB_SM_WEEKDAY];
    before = lit("before") [_val = 0];
    after = lit("after") [_val = HXB_SM_DATETIME_OP_GEQ];

    cond_eidvalue = lit("ip") > is > ipv6_address > ';'
      > lit("eid") > is > eid_value > ';'
      > lit("value") > (equals|lessequal|greaterequal|lessthan|greaterthan|notequal) > float_ > ';';
    cond_timeout = lit("timeout") > is > uint_ > ';';
    cond_datetime = (hour|minute|second|day|month|year|weekday) > (before|after) > uint_ > ';';

    ipv6_address = ((ipv6_address_block [_val+=_1]) > ':' > (ipv6_address_block [_val+=_1]) > ':' >
                   (ipv6_address_block [_val+=_1]) > ':' > (ipv6_address_block [_val+=_1]) > ':' >
                   (ipv6_address_block [_val+=_1]) > ':' > (ipv6_address_block [_val+=_1]) > ':' >
                   (ipv6_address_block [_val+=_1]) > ':' > (ipv6_address_block [_val+=_1]));
    ipv6_address_block = char_("0-9a-fA-F") > char_("0-9a-fA-F") > char_("0-9a-fA-F") > char_("0-9a-fA-F");

    identifier %= eps
      > char_("a-zA-Z_")
      > *char_("a-zA-Z0-9_");
    on_error<rethrow> (
      identifier,
      error_traceback_t("Invalid identifier")
      );

    target_ip = lit("target")
      >> ipv6_address > ';';

    machine_id = lit("machine")
      >> *(char_("0-9a-fA-F")[_val += _1]) > ';';

    startstate %= lit("startstate")
      > identifier
      > ';';

    on_error<rethrow> (
      startstate,
      error_traceback_t("Invalid start state definition")
      );

    condition %=
      lit("condition")
      >> file_pos
      > identifier
      > '{'
      >  ( cond_eidvalue | cond_timeout | cond_datetime )
      > '}'
      ;
    on_error<rethrow> (
      condition,
      error_traceback_t("Invalid condition")
      );

    if_clause %=
      lit("if")
      >> file_pos
      > identifier
      > '{'
      > lit("set") > eid_value > is > float_ > ';'
      > lit("goodstate") > identifier > ';'
      > lit("badstate") > identifier > ';'
      > '}'
      ;
    on_error<rethrow> (
      if_clause,
      error_traceback_t("Invalid if clause in state")
      );

    state %= lit("state")
      >> file_pos
      > identifier
      > '{'
      > *(if_clause)
      > '}'
      ;
    on_error<rethrow> (
      state,
      error_traceback_t("Invalid state definition")
      );

    start %=
      target_ip
      >> machine_id
      >> startstate
      >> +(state | condition)
      > eoi
      ;

    start.name("toplevel");
    identifier.name("identifier");
    state.name("state");
    condition.name("condition");
    startstate.name("start state definition");
    if_clause.name("if clause definition");

    on_error<rethrow> (
      start,
      error_traceback_t("Cannot parse input")
      );
    }

    qi::rule<Iterator, std::string(), Skip> startstate;
    qi::rule<Iterator, std::string(), Skip> target_ip;
    qi::rule<Iterator, std::string(), Skip> machine_id;
    qi::rule<Iterator, state_doc(), Skip> state;
    qi::rule<Iterator, if_clause_doc(), Skip> if_clause;
    qi::rule<Iterator, condition_doc(), Skip> condition;
    qi::rule<Iterator, std::string(), Skip> identifier;
    qi::rule<Iterator, cond_eidvalue_doc(), Skip> cond_eidvalue;
    qi::rule<Iterator, cond_timeout_doc(), Skip> cond_timeout;
    qi::rule<Iterator, cond_datetime_doc(), Skip> cond_datetime;
    qi::rule<Iterator, void(), Skip> is;
    qi::rule<Iterator, int(), Skip> equals;
    qi::rule<Iterator, int(), Skip> lessequal;
    qi::rule<Iterator, int(), Skip> greaterequal;
    qi::rule<Iterator, int(), Skip> lessthan;
    qi::rule<Iterator, int(), Skip> greaterthan;
    qi::rule<Iterator, int(), Skip> notequal;
    qi::rule<Iterator, int(), Skip> hour;
    qi::rule<Iterator, int(), Skip> minute;
    qi::rule<Iterator, int(), Skip> second;
    qi::rule<Iterator, int(), Skip> day;
    qi::rule<Iterator, int(), Skip> month;
    qi::rule<Iterator, int(), Skip> year;
    qi::rule<Iterator, int(), Skip> weekday;
    qi::rule<Iterator, int(), Skip> before;
    qi::rule<Iterator, int(), Skip> after;
    qi::rule<Iterator, std::string(), Skip> ipv6_address;
    qi::rule<Iterator, std::string(), Skip> ipv6_address_block;
    qi::rule<Iterator, unsigned(), Skip> eid_value;
    qi::rule<Iterator, hba_doc(), Skip> start;

    // TODO: Write a custom rule to extract the line number (see eps parser in boost)

    std::vector<std::string>& _error_hints;
    pos_iterator_type& _current_position;

    FileState file_state;
  };
};

#endif /* LIBHBA_HBASM_GRAMMAR_HPP */

