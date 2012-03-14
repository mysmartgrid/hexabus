#include <libhba/common.hpp>
#include <libhba/ast_datatypes.hpp>
#include <libhba/hba_printer.hpp>
#include <libhba/graph_builder.hpp>

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

#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

namespace qi = boost::spirit::qi;
namespace classic = boost::spirit::classic;

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

//struct FileState
//{
//  FileState()
//	: line (1)
//  {}
//
//  int line;
//};
//
//struct UpdateFileInfo
//{
//  UpdateFileInfo (FileState & state)
//	: m_state (state)
//  {}
//
//  template <typename ArgT1, typename ArgT2, typename ArgT3>
//	void operator () (ArgT1 a1, ArgT2 a2, ArgT3 a3) const
//	{
//	  m_state.line++;
//	  std::cout << "updatefileinfo(a1, a2, a3): " << std::endl
//				<< "   " << typeid(a1).name() << std::endl
//				 << "   " << typeid(a2).name() << std::endl
//				<< "   " << typeid(a3).name() << std::endl
//			 ;
//	}
//
//  template <typename IteratorType>
//	void operator () (IteratorType begin, IteratorType end) const
//	{
//	  m_state.line++;
//	  std::cout << "updatefileinfo(b, e): " << std::endl
//			    << "   " << typeid(begin).name() << std::endl
//			    << "   " << typeid(end).name() << std::endl
//																									 ;
//	}
//
//  FileState & m_state;
//};

namespace hexabus
{
  ///////////////////////////////////////////////////////////////////////////
  //  Our comment parser definition
  ///////////////////////////////////////////////////////////////////////////

  template<class Iterator>
	class skipper : public boost::spirit::qi::grammar<Iterator>
  {
	public:
	  skipper():skipper::base_type(start)
	{
	  using namespace boost::spirit::qi;
	  start = space | char_('#') >> *(char_ - eol) >> eol;
	}

	  boost::spirit::qi::rule<Iterator> start;
  };

  ///////////////////////////////////////////////////////////////////////////
  //  Our grammar definition
  ///////////////////////////////////////////////////////////////////////////
  template <typename Iterator>
	struct hexabus_asm_grammar
	: public qi::grammar<Iterator, hba_doc(), skipper<Iterator> >
	{
	  typedef skipper<Iterator> Skip;

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
		using qi::uint_;
		using ascii::string;
		using ascii::space;
		using namespace qi::labels;
		using boost::spirit::_1;
		using boost::spirit::_2;
		using boost::spirit::_3;
		using boost::spirit::_4;
		using boost::spirit::eps;
		using boost::spirit::eoi;

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
		//TODO: Introduce ENUM for operators.
		equals = lit("==") [_val = 1];
		notequals = lit("!=") [_val = 2];
		lessthan = lit("<") [_val = 3];
		greaterthan = lit(">") [_val = 4];
		ipv6_address = (+char_("a-fA-F0-9:") [_val+=_1]);
		eid_value = (uint_ [_val=_1]);

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

		condition %= 
		  lit("condition") [bind(&condition_doc::lineno, _val) = 23]
		  //		  lit("condition") [UpdateFileInfo(file_state)]
		  > identifier
		  > '{'
		  > lit("ip") > is > ipv6_address > ';' 
		  > lit("eid") > is > eid_value > ';' 
		  > lit("value") > (equals|notequals|lessthan|greaterthan) > uint_ > ';' 
		  > '}'
		  ;
		on_error<rethrow> (
			condition,
			error_traceback_t("Invalid condition") 
			);

		if_clause %=
		    lit("if")
		  > identifier
		  > '{'
		  > lit("set") > eid_value > is > uint_ > ';'
		  > lit("goodstate") > identifier > ';'
		  > lit("badstate") > identifier > ';'
		  > '}'
		  ;
		on_error<rethrow> (
			if_clause,
			error_traceback_t("Invalid if clause in state") 
			);

		state %= lit("state") 
		  > identifier
		  > '{'
		  > *(if_clause)
		  > '}'       
		  ;
		on_error<rethrow> (
			state,
			error_traceback_t("Invalid state definition") 
			);

		start %= startstate
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
	  qi::rule<Iterator, state_doc(), Skip> state;
	  qi::rule<Iterator, if_clause_doc(), Skip> if_clause;
	  qi::rule<Iterator, condition_doc(), Skip> condition;
	  //        qi::rule<Iterator, std::string(), Skip> condition_definition;
	  qi::rule<Iterator, std::string(), Skip> identifier;
	  qi::rule<Iterator, void(), Skip> is;
	  qi::rule<Iterator, int(), Skip> equals;
	  qi::rule<Iterator, int(), Skip> notequals;
	  qi::rule<Iterator, int(), Skip> greaterthan;
	  qi::rule<Iterator, int(), Skip> lessthan;
	  qi::rule<Iterator, std::string(), Skip> ipv6_address;
	  qi::rule<Iterator, unsigned(), Skip> eid_value;
	  qi::rule<Iterator, hba_doc(), Skip> start;

	  // TODO: Write a custom rule to extract the line number (see eps parser in boost)

	  std::vector<std::string>& _error_hints;

	  //FileState file_state; 
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

  typedef std::istreambuf_iterator<char> base_iterator_type;
  base_iterator_type in_begin(in);
  typedef boost::spirit::multi_pass<base_iterator_type> forward_iterator_type;
  forward_iterator_type fwd_begin=boost::spirit::make_default_multi_pass(in_begin);
  forward_iterator_type fwd_end;
  typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;
  pos_iterator_type position_begin(fwd_begin, fwd_end, filename);
  pos_iterator_type position_end;

  std::vector<std::string> error_hints;
  typedef hexabus::hexabus_asm_grammar<pos_iterator_type> hexabus_asm_grammar;
  hexabus_asm_grammar grammar(error_hints); // Our grammar
  typedef hexabus::skipper<pos_iterator_type> Skip;
  Skip skipper;
  hexabus::hba_doc ast; // Our tree

  using boost::spirit::ascii::space;
  using boost::spirit::ascii::char_;

  bool r=false;
  try {
	r = phrase_parse(
		// input iterators
		position_begin, position_end,
		// grammar
		grammar, 
		// comment skipper
		//space, //| '#' >> *(char_ - qi::eol) >> qi::eol,
		skipper,
		// output
		ast
		);
  } catch(const qi::expectation_failure<pos_iterator_type>& e) {
	const classic::file_position_base<std::string>& pos = e.first.get_position();
	std::cerr << "Error in " << pos.file <<
	  " line " << pos.line << " column " << pos.column << std::endl <<
	  "'" << e.first.get_currentline() << "'" << std::endl
	  << std::setw(pos.column) << " " 
	  << "^- " << *error_traceback_t::stack.begin()<< std::endl;
//	std::cout << "parser backtrace: " << std::endl;
//	std::vector<std::string>::iterator it;
//	for(  it = error_traceback_t::stack.begin();
//		it != error_traceback_t::stack.end(); ++it ) 
//	{
//	  std::cout << "- " << (*it) << std::endl;
//	}
  } catch (const std::runtime_error& e) {
	std::cout << "Exception occured: " << e.what() << std::endl;
  }

  if (r && position_begin == position_end) {
	std::cout << "-------------------------\n";
	std::cout << "Parsing succeeded\n";
	std::cout << "-------------------------\n";
	//hexabus::hba_printer printer;
	//printer(ast);
	hexabus::GraphBuilder gBuilder;
	gBuilder(ast);
	std::ofstream ofs;
	ofs.open("foo.dot");

	//graph_t g=gBuilder.get_graph();
	gBuilder.write_graphviz(ofs);
	ofs.close();
	return 0;
  } else {
	if (!r)
	  std::cout << "Parsing failed." << std::endl;
	if (r)
	  std::cout << "Parsing failed: we haven't "
		<< "reached the end of the input." << std::endl;
	return 1;
  }
} 

