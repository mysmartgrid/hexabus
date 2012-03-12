//#define BOOST_SPIRIT_USE_PHOENIX_V3 1
#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>
#include <boost/foreach.hpp>
#include <boost/spirit/home/qi.hpp> 
#include <boost/spirit/home/support/info.hpp> 
#include <boost/spirit/home/phoenix.hpp> 
#include <boost/spirit/home/phoenix/statement/sequence.hpp> 
#include <boost/spirit/include/support_multi_pass.hpp>
#include <boost/spirit/include/classic_position_iterator.hpp>

#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

namespace qi = boost::spirit::qi;
namespace classic = boost::spirit::classic;

namespace hexabus
{
    namespace fusion = boost::fusion;
    namespace phoenix = boost::phoenix;
    namespace ascii = boost::spirit::ascii;

    ///////////////////////////////////////////////////////////////////////////
    //  HexaBus assembler representation
    ///////////////////////////////////////////////////////////////////////////
    struct state_doc {
      std::string name;
      std::string state_string;
    };

    struct condition_doc {
      std::string name;
      std::string condition_string;
      //std::vector<clause_doc> clauses;
    };

    typedef
        boost::variant<
          state_doc              // one state definition
        , condition_doc          // one condition definition
        >
    hba_doc_block;

    struct hba_doc
    {
        std::string start_state;
        std::vector<hba_doc_block> blocks;                           
    };
}

// We need to tell fusion about our hba struct
// to make it a first-class fusion citizen
BOOST_FUSION_ADAPT_STRUCT(
    hexabus::state_doc,
    (std::string, name)
    (std::string, state_string)
    //(std::vector<hexabus::mini_xml_node>, children)
)

BOOST_FUSION_ADAPT_STRUCT(
    hexabus::condition_doc,
    (std::string, name)
    (std::string, condition_string)
    //(std::vector<hexabus::clause_doc>, clauses) 
)

BOOST_FUSION_ADAPT_STRUCT(
    hexabus::hba_doc,
    (std::string, start_state)
    (std::vector<hexabus::hba_doc_block>, blocks)
    //(std::vector<hexabus::mini_xml_node>, children)
)

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

namespace hexabus
{
    ///////////////////////////////////////////////////////////////////////////
    //  Print out the HexaBus Assembler
    ///////////////////////////////////////////////////////////////////////////
    int const tabsize = 4;

    void tab(int indent)
    {
        for (int i = 0; i < indent; ++i)
            std::cout << ' ';
    }

    struct hba_printer
    {
        hba_printer(int indent = 0)
          : indent(indent)
        {
        }

        void operator()(hba_doc const& xml) const;

        int indent;
    };

    struct mini_xml_node_printer : boost::static_visitor<>
    {
        mini_xml_node_printer(int indent = 0)
          : indent(indent)
        {
        }

        void operator()(state_doc const& xml) const
        {
            tab(indent);
            std::cout << "state: " << xml.name << std::endl;
            std::cout << "body: " << xml.state_string << std::endl;
            tab(indent);
        }


        void operator()(condition_doc const& xml) const
        {
          tab(indent);
          std::cout << "condition: " << xml.name << std::endl;
          std::cout << "body: " << xml.condition_string << std::endl;

          
          tab(indent);
        }


        void operator()(std::string const& text) const
        {
          tab(indent+tabsize);
          std::cout << "text: \"" << text << '"' << std::endl;
        }

        int indent;
    };

    void hba_printer::operator()(hba_doc const& xml) const
    {
      tab(indent);
      std::cout << "start state: " << xml.start_state << std::endl;
      tab(indent);
      std::cout << '{' << std::endl;

      BOOST_FOREACH(hba_doc_block const& block, xml.blocks)
      {
        //    mini_xml_node_printer p(indent);
        //    p(block);
        boost::apply_visitor(mini_xml_node_printer(indent), block);
      }

      tab(indent);
      std::cout << '}' << std::endl;
    }

    ///////////////////////////////////////////////////////////////////////////
    //  Our grammar definition
    ///////////////////////////////////////////////////////////////////////////
    template <typename Iterator>
      struct hexabus_asm_grammar
      : public qi::grammar<Iterator, hba_doc(), ascii::space_type>
      {
        hexabus_asm_grammar(std::vector<std::string>& error_hints)
          : hexabus_asm_grammar::base_type(start),
          _error_hints(error_hints)
        {
          using qi::lit;
          using qi::lexeme;
          using qi::on_error;
          using qi::rethrow;
          using qi::fail;
          using qi::accept;
          using ascii::char_;
          using ascii::string;
          using ascii::space;
          using namespace qi::labels;
          using boost::spirit::_4;
          using boost::spirit::eps;

          using phoenix::construct;
          using phoenix::val;
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

          identifier %= eps
            > char_("a-zA-Z_") 
            > *char_("a-zA-Z0-9_");
          on_error<rethrow> (
              identifier,
              error_traceback_t("Invalid identifier")
              );

          startstate %= lit("startstate") 
            > identifier
            > ';'
            ;
          on_error<rethrow> (
              startstate,
              error_traceback_t("Invalid start state definition")
              );

          condition_definition %= lexeme[+(char_ - '}')];
          on_error<rethrow> (
              condition_definition,
              error_traceback_t("Invalid condition body") 
              );

          condition %= lit("condition") 
            > identifier
            > '{'
            > condition_definition 
            > '}'
            ;
          on_error<rethrow> (
              condition,
              error_traceback_t("Invalid condition") 
              );

          state_definition %= lexeme[+(char_ - '}')];
          on_error<rethrow> (
              state_definition,
              error_traceback_t("Invalid state body") 
              );

          state %= lit("state") 
            > identifier
            > '{'
            > state_definition
            > '}'       //[TODO: mark line for this state]
            ;
          on_error<rethrow> (
              state,
              error_traceback_t("Invalid state definition") 
              );

          start %= startstate
            > +(state | condition);

          start.name("toplevel");
          identifier.name("identifier");
          state.name("state");
          condition.name("condition");
          condition_definition.name("condition definition");
          startstate.name("start state definition");
          state_definition.name("state definition");

          on_error<rethrow> (
              start,
              error_traceback_t("Cannot parse input")
              );
        }

        qi::rule<Iterator, std::string(), ascii::space_type> startstate;
        qi::rule<Iterator, state_doc(), ascii::space_type> state;
        qi::rule<Iterator, std::string(), ascii::space_type> state_definition;
        qi::rule<Iterator, condition_doc(), ascii::space_type> condition;
        qi::rule<Iterator, std::string(), ascii::space_type> condition_definition;
        qi::rule<Iterator, std::string(), ascii::space_type> identifier;
        qi::rule<Iterator, hba_doc(), ascii::space_type> start;

        std::vector<std::string>& _error_hints;

      };
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
  char const* filename;
  if (argc > 1) {
    filename = argv[1];
  } else {
    std::cerr << "Error: No input file provided." << std::endl;
    return 1;
  }

  std::ifstream in(filename, std::ios_base::in);

  if (!in) {
    std::cerr << "Error: Could not open input file: "
      << filename << std::endl;
    return 1;
  }

  in.unsetf(std::ios::skipws); // No white space skipping!

  // iterate over stream input
  typedef std::istreambuf_iterator<char> base_iterator_type;
  base_iterator_type in_begin(in);

  // convert input iterator to forward iterator, usable by spirit parser
  typedef boost::spirit::multi_pass<base_iterator_type> forward_iterator_type;
  forward_iterator_type fwd_begin = boost::spirit::make_default_multi_pass(in_begin);
  forward_iterator_type fwd_end;

  // wrap forward iterator with position iterator, to record the position
  typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;
  pos_iterator_type position_begin(fwd_begin, fwd_end, filename);
  pos_iterator_type position_end;


  //std::string storage; // We will read the contents here.
  //std::copy(
  //    std::istream_iterator<char>(in),
  //    std::istream_iterator<char>(),
  //    std::back_inserter(storage));

  //typedef hexabus::hexabus_asm_grammar<std::string::const_iterator> hexabus_asm_grammar;
  std::vector<std::string> error_hints;
  typedef hexabus::hexabus_asm_grammar<pos_iterator_type> hexabus_asm_grammar;
  hexabus_asm_grammar grammar(error_hints); // Our grammar
  hexabus::hba_doc ast; // Our tree

  using boost::spirit::ascii::space;
  using boost::spirit::ascii::char_;

  //std::string::const_iterator iter = storage.begin();
  //std::string::const_iterator end = storage.end();
  bool r=false;
  try {
    r = phrase_parse(
        // input iterators
        //iter, end, 
        position_begin, position_end,
        // grammar
        grammar, 
        // comment skipper
        // qi-space!!! SKIPPER qi::space | qi::eol 
        space, //| '#' >> *(char_ - qi::eol) >> qi::eol,
        // output
        ast);
  } catch(const qi::expectation_failure<pos_iterator_type>& e) {
    const classic::file_position_base<std::string>& pos = e.first.get_position();
    std::cerr << "Error in " << pos.file <<
      " line " << pos.line << " column " << pos.column << std::endl <<
      "'" << e.first.get_currentline() << "'" << std::endl
      << std::setw(pos.column) << " " 
      << "^- " << *error_traceback_t::stack.begin()<< std::endl;
    std::cout << "parser backtrace: " << std::endl;
    std::vector<std::string>::iterator it;
    for(  it = error_traceback_t::stack.begin();
        it != error_traceback_t::stack.end(); ++it ) 
    {
      std::cout << "- " << (*it) << std::endl;
    }
  } catch (const std::runtime_error& e) {
    std::cout << "Exception occured: " << e.what() << std::endl;
  }

  hexabus::hba_printer printer;
  if (r && position_begin == position_end) {
    std::cout << "-------------------------\n";
    std::cout << "Parsing succeeded\n";
    std::cout << "-------------------------\n";
    printer(ast);
    return 0;
  } else {
	if (!r)
	  std::cout << "Parsing failed." << std::endl;
	if (r)
	  std::cout << "Parsing succeded, but we haven't "
		<< "reached the end of the input." << std::endl;
    return 1;
  }

  //  else {
  //    //std::string::const_iterator some = position_begin+30;
  //    //std::string context(position_begin, (some>position_end)?position_end:some);
  //    std::cout << "-------------------------\n";
  //    std::cout << "Parsing failed\n";
  //    //std::cout << "stopped at: " << context << std::endl;
  //    std::cout << "-------------------------\n";
  //    printer(ast);
  //    return 1;
  //  }
} 

