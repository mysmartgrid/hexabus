#include <libhbc/common.hpp>
#include <libhbc/ast_datatypes.hpp>
#include <libhbc/skipper.hpp>
#include <libhbc/hbcomp_grammar.hpp>
#include <libhbc/hbc_printer.hpp>
#include <libhbc/filename_annotation.hpp>
#include <libhbc/graph_builder.hpp>
#include <libhbc/table_builder.hpp>
#include <libhbc/module_instantiation.hpp>
#include <libhbc/hba_output.hpp>
#include <libhbc/graph_transformation.hpp>
#include <libhbc/include_handling.hpp>

// commandline parsing.
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <iostream>
#include <fstream>
#include <istream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

int main(int argc, char** argv)
{
  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " [-i] inputfile [options]";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version,v", "display libhbc version")
    ("verbose,V", "show more diagnostic output")
    ("input,i", po::value<std::string>(), "the input file")
    ("print,p", "print parsed version of the input file")
    ("graph,g", po::value<std::string>(), "output the program graph in graphviz format")
    ("tables,t", "print endpoint and alias tables")
    ("output,o", "output Hexabus Assembler (HBA) code")
    ("slice,s", "slice state machines")
  ;
  po::positional_options_description p;
  p.add("input", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
  po::notify(vm);

  std::string infile;
  if(vm.count("help"))
  {
    hexabus::VersionInfo vinf;
    std::cout << "Hexbus Compiler. libhbc version " << vinf.getVersion() << "." << std::endl << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  bool verbose = false;
  if(vm.count("verbose")) {
    verbose = true;
  }

  if(vm.count("version") || verbose) {
    hexabus::VersionInfo vinf;
    std::cout << "Hexbus Compiler. libhbc version " << vinf.getVersion() << "." << std::endl;
    return 1;
  }

  if(!vm.count("input"))
  {
    std::cerr << "Error: You must specify an input file." << std::endl;
    return 1;
  } else {
    infile = vm["input"].as<std::string>();
  }

  hexabus::IncludeHandling includes(infile); // give filename specified on the command line to IncludeHandling

  hexabus::hbc_doc ast; // The AST - all the files get parsed into one ast
  bool okay = true;

  // read file, handle includes, read includes, repeat until all included files are parsed (or we find an error)
  for(unsigned int f = 0; f < includes.size() && okay; f++) {
    bool r = false;
    std::ifstream in(includes[f].string().c_str(), std::ios_base::in);
    if(verbose)
      std::cout << "Reading input file " << includes[f].string() << "..." << std::endl;

    if(!in) {
      std::cerr << "Error: Could not open input file: " << includes[f].string() << std::endl;
      return 1;
    }

    in.unsetf(std::ios::skipws); // no white space skipping, this will be handled by the parser

    typedef std::istreambuf_iterator<char> base_iterator_type;
    typedef boost::spirit::multi_pass<base_iterator_type> forward_iterator_type;
    typedef classic::position_iterator2<forward_iterator_type> pos_iterator_type;

    base_iterator_type in_begin(in);
    forward_iterator_type fwd_begin = boost::spirit::make_default_multi_pass(in_begin);
    forward_iterator_type fwd_end;
    pos_iterator_type position_begin(fwd_begin, fwd_end, includes[f].string());
    pos_iterator_type position_end;

    std::vector<std::string> error_hints;
    typedef hexabus::hexabus_comp_grammar<pos_iterator_type> hexabus_comp_grammar;
    typedef hexabus::skipper<pos_iterator_type> Skip;
    hexabus_comp_grammar grammar(error_hints, position_begin);
    Skip skipper;

    using boost::spirit::ascii::space;
    using boost::spirit::ascii::char_;

    try {
      r = phrase_parse(position_begin, position_end, grammar, skipper, ast);
    } catch(const qi::expectation_failure<pos_iterator_type>& e) {
      const classic::file_position_base<std::string>& pos = e.first.get_position();
      std::cerr << "Error in " << pos.file << " line " << pos.line << " column " << pos.column << std::endl
        << "'" << e.first.get_currentline() << "'" << std::endl
        << std::setw(pos.column) << " " << "^-- " << *hexabus::error_traceback_t::stack.begin() << std::endl;
    } catch(const std::runtime_error& e) {
      std::cout << "Exception occured: " << e.what() << std::endl;
    }

    if(r && position_begin == position_end) {
      if(verbose)
        std::cout << "Parsing of file " << includes[f] << " succeeded." << std::endl;

      // put the current file name into all the parts of the AST which don't have a filename yet.
      hexabus::FilenameAnnotation an(includes[f].string());
      an(ast);

      // Find includes and add them to file name list
      BOOST_FOREACH(hexabus::hbc_block block, ast.blocks) {
        if(block.which() == 0) { // include_doc
          includes.addFileName(boost::get<hexabus::include_doc>(block));
        }
      }
    } else {
      okay = false;
      if(!r)
        std::cout << "Parsing of file " << includes[f].string() << " failed." << std::endl;
      if(r)
        std::cout << "Parsing of file " << includes[f].string() << " failed: Did not reach end of file." << std::endl;
    }
  }

  if(okay) {
    if(vm.count("print")) {
      hexabus::hbc_printer printer;
      printer(ast);
    }

    // build endpoint and device alias tables
    if(verbose)
      std::cout << "Building device / module / endpoint tables..." << std::endl;
    hexabus::TableBuilder tableBuilder;
    tableBuilder(ast);
    if(vm.count("tables")) {
      tableBuilder.print();
    }

    // build module instances
    if(verbose)
      std::cout << "Instantiating modules..." << std::endl;
    hexabus::ModuleInstantiation modules(tableBuilder.get_module_table(), tableBuilder.get_device_table(), tableBuilder.get_endpoint_table());
    modules(ast);

    // build "big" state machine graph
    if(verbose)
      std::cout << "Building state machine graph..." << std::endl;
    hexabus::GraphBuilder gBuilder;
    gBuilder(ast);
    if(vm.count("graph")) {
      std::ofstream ofs;
      std::string outfile(vm["graph"].as<std::string>());
      if(std::string("") == outfile) {
        std::cout << "No graph output file specified." << std::endl;
        exit(-1);
      }
      ofs.open(outfile.c_str());
      if(!ofs) {
        std::cerr << "Error: Could not open graph output file." << std::endl;
        exit(-1);
      }
      gBuilder.write_graphviz(ofs);
      ofs.close();
    }

    // TODO this has to be automated once we know how we want to do it
    if(vm.count("slice")) {
      if(verbose)
        std::cout << "Slicing state machine graph..." << std::endl;
      hexabus::GraphTransformation gt(tableBuilder.get_device_table(), tableBuilder.get_endpoint_table());
      gt(gBuilder.get_graph());
    }

    // TODO this has to be automated as well
    if(vm.count("output")) {
      if(verbose)
        std::cout << "Generating Hexabus Assembler output..." << std::endl;
      hexabus::HBAOutput out(gBuilder.get_graph(), tableBuilder.get_device_table(), tableBuilder.get_endpoint_table());
      out(std::cout);
    }

  } else {
    return 1;
  }

  return 0;
}
