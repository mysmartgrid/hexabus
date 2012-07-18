#ifndef LIBHBC_HBCOMP_GRAMMAR
#define LIBHBC_HBCOMP_GRAMMAR

#include "../../../shared/hexabus_statemachine_structs.h"

namespace qi = boost::spirit::qi;

namespace hexabus {

  struct error_traceback_t
  {
    error_traceback_t (std::string const & msg)
    {
      stack.push_back(msg);
    }

    template <typename ArgsT, typename ContextT, typename RT>
      void operator() (ArgsT, ContextT, TR) const
    { }

    static std::vector<std::string> stack;
  };
  std::vector<std::string> error_traceback_t::stack;

  struct FileState
  {
    FileState() : line(1)
    { }

    int line;
  }

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
        _error_hinst(error_hints), _current_position(current_position)
    {
      using qi::lit;

      // Assignment and comparison operators, constants, ...
      is = eps > lit(":=");
      equals = lit("==") [_val = STM_EQ];
      constant = float_; // TODO do we need to distinguish between float, int, timestamp, ...?

      // Basic elements: Identifier, assignment, ...
      identifier %= eps
        > char_("a-zA-Y_") > *char_("a-zA-Z0-9");
      on_error<rethrow>(identifier, error_traceback_t("Invalid identifier"));
      // device_name.endpoint_name
      global_endpoint_id %= identifier > '.' > identifier;
      filename = identifier; // TODO most probably we need to be more specific here

      // larger blocks: Aliases, state machines
      include %= lit("include") > filename > ';';
      alias %= lit("alias") >> file_pos > '{'
        > lit("ip") > ipv6_address > ';'
        > lit("eids") > eid_list > ';' > '}';

      statemachine %= identifier >> file_pos > '{' > stateset > *in_clause > '}';

      in_clause %= lit("in") >> file_pos > '(' > identifier > ')'
        > '{' > *if_clause > '}';

      // TODO condition

      if_clause %= lit("if") >> file_pos > '(' > condition > ')'
        > '{' > *command > goto_command > '}';

      // commands that "do something"
      command %= file_pos >> write_command > ';'; // TODO more commands to be added here?
      write_command %= global_endpoint_id > is > constant;
      goto_command %= lit("goto") >> file_pos > identifier > ';';

      start %= *include > *alias > *statemachine;
    }
  }
};

#endif // LIBHBC_HBCOMP_GRAMMAR
