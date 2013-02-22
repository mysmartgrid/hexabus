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
#include <algorithm>

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
    ("verbose,V", "print more inforamtion onto the console during output generation")
    ("print,p", "print parsed version of the input file")
    ("graph,g", po::value<std::string>(), "generate a dot file")
    ("input,i", po::value<std::string>(), "the hexabus assembler input file")
    ("output,o", po::value<std::string>(), "the hexabus assembler output file")
    ("dtdef,d", po::value<std::string>(), "data type definition file")
    ;
  po::positional_options_description p;
  p.add("input", 1);

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

  std::string dt_filename = "";
  if (vm.count("dtdef")) {
    dt_filename = vm["dtdef"].as<std::string>();
  } else {
    std::cout << "Warning: No data type definition file specified. Output will be generated with blank data types." << std::endl;
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

	// we're done parsing - close input file
	in.close();

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
      /* try {
        gChecks.check_states_incoming();
        gChecks.check_states_outgoing();
        gChecks.check_conditions_incoming();
        gChecks.check_conditions_outgoing();
      } catch (hexabus::GenericException& ge) {
        std::cout << "ERROR: " << ge.what() << std::endl;
        exit(-1);
      } */

      bool verbose = false;
      if(vm.count("verbose"))
        verbose = true;

      hexabus::generator_flash gf(gBuilder.get_graph(), ast, dt_filename, verbose);

      std::ofstream ofs;
      std::string outfile(vm["output"].as<std::string>());
      if (std::string("") == outfile) {
        std::cout << "No output file specified." << std::endl;
        exit(-1);
      }

      ofs.open(outfile.c_str());
      if (!ofs) {
        std::cerr << "Error: Could not open output file: "
          << outfile << std::endl;
        return 1;
      }

      std::vector<uint8_t> data;

      // first, populate data with target IP address
      for(size_t i = 0; i < ast.target_ip.size() / 2; ++i) {
        std::stringstream ss;
        unsigned int ipbyte;
        ss << std::hex << ast.target_ip.substr(2 * i, 2); // read two bytes from the string, since two hex digits correspond to one byte in binary
        ss >> ipbyte;
        data.push_back(ipbyte);
      }

			// then add the device name
			boost::array<char,64> dev_name; // make an array that's as long as a chunk of the uploader
			BOOST_FOREACH(char c, dev_name) { // fill it with zeros
				c = '\0';
			}
			for(size_t i = 0; i < std::min((size_t)30, ast.device_name.size()); ++i) { // copy device name from AST to the array (only first 30 bytes, we don't have more space in the EEPROM
				dev_name[i] = ast.device_name[i];
			}
			BOOST_FOREACH(char c, dev_name) { // copy device name from the array to the output vector
				data.push_back(c);
			}

      // then add machine ID
      if(ast.machine_id.size() > 32)
        ast.machine_id = ast.machine_id.substr(0,32); // truncate if longer than 32 characters
      while(ast.machine_id.size() < 32)
        ast.machine_id += "0"; // pad with zeros if it's shorter than 32 characters

      // now convert to single bytes from the hex representation, and store it into the output
      for(size_t i = 0; i < ast.machine_id.size(); i += 2) {
        std::stringstream s;
        unsigned int c;
        s << std::hex << ast.machine_id[i] << ast.machine_id[i+1];
        s >> c;
        data.push_back((char)c);
      }

      gf(data);

      /*std::string cond_b64str(hexabus::to_base64(cond_data));
      std::string trans_b64str(hexabus::to_base64(trans_data));
      std::string dttrans_b64str(hexabus::to_base64(dttrans_data));
      cond_ofs << "1" << std::endl << cond_b64str;
      trans_ofs << "2" << std::endl << trans_b64str;
      dttrans_ofs << "3" << std::endl << dttrans_b64str;*/

      ofs << std::string(data.begin(), data.end());

      ofs.close();
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

