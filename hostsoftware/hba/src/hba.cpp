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
	unsigned int lineno;
	std::string name;
	std::string ipv6_address;
	unsigned int eid;
	unsigned int op;
	unsigned int value;
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
	(std::string, ipv6_address)
	(unsigned int, eid)
	(unsigned int, op)
	(unsigned int, value)
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
	  std::cout << "condition (line " << xml.lineno << "): " << xml.name << std::endl;
	  tab(tabsize);
	  std::cout << "IPv6 addr: " << xml.ipv6_address;
	  // for( unsigned int i = 0; i < xml.ipv6_address.size(); i += 1) {
	  //   std::cout << std::setw(2) << std::setfill('0') 
	  //     << std::hex << (int)(xml.ipv6_address[i] & 0xFF);
	  // }
	  std::cout << ", size=" << xml.ipv6_address.size() << std::endl;
	  tab(tabsize);
	  std::cout << "EID: " << xml.eid << std::endl;
	  tab(tabsize);
	  std::cout << "Operator: " << xml.op << std::endl;
	  tab(tabsize);
	  std::cout << "Value: " << xml.value << std::endl;
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
	//: public qi::grammar<Iterator, hba_doc(), ascii::space_type> 
	: public qi::grammar<Iterator, hba_doc(), skipper<Iterator> >
	{
	  typedef skipper<Iterator> Skip;
	  //typedef ascii::space_type Skip;

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
		  >> +(state | condition)
		  > eoi
		  ;

		start.name("toplevel");
		identifier.name("identifier");
		state.name("state");
		condition.name("condition");
		//         condition_definition.name("condition definition");
		startstate.name("start state definition");
		state_definition.name("state definition");

		on_error<rethrow> (
			start,
			error_traceback_t("Cannot parse input")
			);
	  }

	  qi::rule<Iterator, std::string(), Skip> startstate;
	  qi::rule<Iterator, state_doc(), Skip> state;
	  qi::rule<Iterator, std::string(), Skip> state_definition;
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

