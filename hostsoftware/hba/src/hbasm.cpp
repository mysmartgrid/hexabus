#include <libhba/common.hpp>
#include <libhba/error.hpp>
#include <libhba/ast_datatypes.hpp>
#include <libhba/hba_printer.hpp>
#include <libhba/graph_builder.hpp>
#include <libhba/graph_checks.hpp>
#include <libhba/skipper.hpp>
#include <libhba/hbasm_grammar.hpp>
#include <libhba/generator_flash.hpp>
#include <libhba/base64.hpp>

// commandline parsing.
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>



///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{

  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " [-i] inputfile [options]";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version,v", "print libklio version and exit")
    ("print,p", "print parsed version of the input file")
    ("graph,g", po::value<std::string>(), "generate a dot file")
    ("input,i", po::value<std::string>(), "the hexabus assembler input file")
    ("output,o", po::value<std::string>(), "the hexabus assembler output file")
    ;
  po::positional_options_description p;
  p.add("infile", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
      options(desc).positional(p).run(), vm);
  po::notify(vm);

  // Begin processing of commandline parameters.
  std::string infile;
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }
  if (vm.count("version")) {
    hexabus::VersionInfo::Ptr vi(new hexabus::VersionInfo());
    std::cout << "hexabus assembler version " << vi->getVersion() << std::endl;
    return 0;
  }
  if (! vm.count("input")) {
    std::cerr << "Error: You must specify an input file." << std::endl;
    return 1;
  } else {
    infile=vm["input"].as<std::string>();
  }

  std::ifstream in(infile.c_str(), std::ios_base::in);

  if (!in) {
	std::cerr << "Error: Could not open input file: "
	  << infile << std::endl;
	return 1;
  } 

  in.unsetf(std::ios::skipws); // No white space skipping!

  typedef std::istreambuf_iterator<char> base_iterator_type;
  base_iterator_type in_begin(in);
  typedef boost::spirit::multi_pass<base_iterator_type> forward_iterator_type;
  forward_iterator_type fwd_begin=boost::spirit::make_default_multi_pass(in_begin);
  forward_iterator_type fwd_end;
  typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;
  pos_iterator_type position_begin(fwd_begin, fwd_end, infile);
  pos_iterator_type position_end;

  std::vector<std::string> error_hints;
  typedef hexabus::hexabus_asm_grammar<pos_iterator_type> hexabus_asm_grammar;
  hexabus_asm_grammar grammar(error_hints, position_begin); // Our grammar
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
	  << "^- " << *hexabus::error_traceback_t::stack.begin()<< std::endl;
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
    if (vm.count("print")) {
      hexabus::hba_printer printer;
      printer(ast);
    } 
    else if (vm.count("graph")) {
      hexabus::GraphBuilder gBuilder;
      gBuilder(ast);
      hexabus::GraphChecks gChecks(gBuilder.get_graph());
      try {
        gChecks.check_states_incoming();
        gChecks.check_states_outgoing();
        gChecks.check_conditions_incoming();
        gChecks.check_conditions_outgoing();
      } catch (hexabus::GenericException& ge) {
        std::cout << "ERROR: " << ge.what() << std::endl;
        exit(-1);
      }

      std::ofstream ofs;
      std::string outfile(vm["graph"].as<std::string>());
      if (std::string("") == outfile) {
        std::cout << "No graph output file specified." << std::endl;
        exit(-1);
      }
      ofs.open(outfile.c_str());
      if (!ofs) {
        std::cerr << "Error: Could not open graph output file: "
          << outfile << std::endl;
        return 1;
      }
      gBuilder.write_graphviz(ofs);
      ofs.close();
    }
    else if (vm.count("output")) {
      hexabus::GraphBuilder gBuilder;
      gBuilder(ast);
      hexabus::GraphChecks gChecks(gBuilder.get_graph());
      try {
        gChecks.check_states_incoming();
        gChecks.check_states_outgoing();
        gChecks.check_conditions_incoming();
        gChecks.check_conditions_outgoing();
      } catch (hexabus::GenericException& ge) {
        std::cout << "ERROR: " << ge.what() << std::endl;
        exit(-1);
      }
      hexabus::generator_flash gf(gBuilder.get_graph(), ast);

      std::ofstream cond_ofs;
      std::ofstream trans_ofs;
      std::ofstream dttrans_ofs;
      std::string outfile(vm["output"].as<std::string>());
      if (std::string("") == outfile) {
        std::cout << "No output file specified." << std::endl;
        exit(-1);
      }
      std::string cond_outfile(outfile);
      std::string trans_outfile(outfile);
      std::string dttrans_outfile(outfile);

      cond_outfile += ".cond";
      trans_outfile += ".trans";
      dttrans_outfile += ".dttrans";

      cond_ofs.open(cond_outfile.c_str());
      if (!cond_ofs) {
        std::cerr << "Error: Could not open output file: "
          << cond_outfile << std::endl;
        return 1;
      }
      trans_ofs.open(trans_outfile.c_str());
      if (!trans_ofs) {
        std::cerr << "Error: Could not open output file: "
          << trans_outfile << std::endl;
        return 1;
      }
      dttrans_ofs.open(dttrans_outfile.c_str());
      if (!dttrans_ofs) {
        std::cerr << "Error: Could not open output file: "
          << dttrans_outfile << std::endl;
        return 1;
      }

      std::vector<uint8_t> cond_data;
      std::vector<uint8_t> trans_data;
      std::vector<uint8_t> dttrans_data;
      
      gf(cond_data, trans_data, dttrans_data);
      //gf(data, data, data); // TODO maybe we want to write to three different files here, or figure out some other way to store this

      std::string cond_b64str(hexabus::to_base64(cond_data));
      std::string trans_b64str(hexabus::to_base64(trans_data));
      std::string dttrans_b64str(hexabus::to_base64(dttrans_data));
      cond_ofs << "1" << std::endl << cond_b64str;
      trans_ofs << "2" << std::endl << trans_b64str;
      dttrans_ofs << "3" << std::endl << dttrans_b64str;

      cond_ofs.close();
      trans_ofs.close();
      dttrans_ofs.close();
    } else {
      std::cout << "Parsing succeeded." << std::endl;
    }
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

