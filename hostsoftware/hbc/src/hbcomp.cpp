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
#include <libhbc/graph_simplification.hpp>
#include <libhbc/graph_checks.hpp>
#include <libhbc/dtype_file_generator.hpp>
#include <libhbc/dtype_file_generator.hpp>

// commandline parsing.
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

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
    ("graph,g", po::value<std::string>(), "output file for the program graph in graphviz format")
    ("tables,t", "print endpoint and alias tables")
    ("slice,s", po::value<std::string>(), "file name prefix for sliced state machine graphs")
    ("simplification,f", po::value<std::string>(), "file name prefix for simplified state machine graphs")
    ("basiccheck,c", "Perform basic graph checks (no incoming / no outgoing / unreachable)")
    ("checkmachine,m", po::value<std::string>(), "name of machine on which to perform liveness check")
    ("always,a", po::value<std::string>(), "perform liveness check ('node is always reachable') for node")
    ("never,n", po::value<std::string>(), "perform safety check ('node is never reachable') for node")
    ("output,o", po::value<std::string>(), "file name prefix for Hexabus Assembler (HBA) output")
    ("datatypefile,d", po::value<std::string>(), "name of data type file to be generated")
    ;
  po::positional_options_description p;
  p.add("input", 1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
  po::notify(vm);

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
    hexabus::GlobalOptions::getInstance()->setVerbose(true);
  } else {
    hexabus::GlobalOptions::getInstance()->setVerbose(false);
  }

  if(vm.count("version")) {
    hexabus::VersionInfo vinf;
    std::cout << "Hexbus Compiler. libhbc version " << vinf.getVersion() << "." << std::endl;
    return 1;
  }

  std::string infile;
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

    // slice state machine graph
    if(verbose)
      std::cout << "Building separate state machine graphs for devices..." << std::endl;
    hexabus::GraphTransformation gt(tableBuilder.get_device_table(), tableBuilder.get_endpoint_table(), gBuilder.getMachineFilenameMap());
    gt(gBuilder.get_graph());

    if(vm.count("slice")) {
      if(verbose)
        std::cout << "Writing device state machine graph files..." << std::endl;
      gt.writeGraphviz(vm["slice"].as<std::string>());
    }

    if(verbose)
      std::cout << "Simplifying device state machine graphs..." << std::endl;
    hexabus::GraphSimplification gs(gt.getDeviceGraphs());
    gs();

    if(vm.count("simplification")) {
      if(verbose)
        std::cout << "Writing simplified device state machine graph files..." << std::endl;
      gt.writeGraphviz(vm["simplification"].as<std::string>());
    }

    hexabus::GraphChecks gc(gBuilder.get_graph());
    if(vm.count("basiccheck")) {
      if(verbose)
        std::cout << "Performing basic graph checks..." << std::endl;
      gc.find_states_without_inbound();
      gc.find_states_without_outgoing();
      gc.find_unreachable_states();
    }

    if(vm.count("checkmachine")) {
      if(verbose)
        std::cout << "Performing advanced graph checks..." << std::endl;
      std::vector<std::string> states;
      // "always"-check
      if(vm.count("always")) {
        if(verbose)
          std::cout << "...'always' check." << std::endl;
        boost::split(states, vm["always"].as<std::string>(), boost::is_any_of(","));
        BOOST_FOREACH(std::string state, states) {
          gc.reachable_from_anywhere(state, vm["checkmachine"].as<std::string>());
        }
      }
      // "never"-check
      if(vm.count("never")) {
        if(verbose)
          std::cout << "...'never' check." << std::endl;
        boost::split(states, vm["never"].as<std::string>(), boost::is_any_of(","));
        BOOST_FOREACH(std::string state, states) {
          gc.never_reachable(state, vm["checkmachine"].as<std::string>());
        }
      }
    }

    if(!vm.count("output"))
      std::cout << "No output file name specified (option -o not given) - no HBA output will be written." << std::endl;

    std::map<std::string, hexabus::graph_t_ptr> graphs = gt.getDeviceGraphs(); // we can take the graph map from gt again, because GraphSimplification works in-place
    for(std::map<std::string, hexabus::graph_t_ptr>::iterator graphIt = graphs.begin(); graphIt != graphs.end(); graphIt++) {
      hexabus::HBAOutput out(graphIt->first, graphIt->second, tableBuilder.get_device_table(), tableBuilder.get_endpoint_table(), gBuilder.getMachineFilenameMap());

      if(vm.count("output")) {
        std::ostringstream fnoss;
        fnoss << vm["output"].as<std::string>() << graphIt->first << ".hba";

        std::ofstream ofs;
        ofs.open(fnoss.str().c_str());

        if(!ofs) {
          std::cerr << "Could not open HBA output file " << fnoss << "." << std::endl;
          exit(-1);
        }
        out(ofs);
        ofs.close();
      } else { // don't generate output, just write to a "dummy" stream (so that all the checks, names etc. are performed but no output is generated
        std::ostringstream oss;
        out(oss);
      }
    }

    if(vm.count("datatypefile")) {
      if(verbose)
        std::cout << "Generating data type file..." << std::endl;

      std::ofstream dtofs;
      dtofs.open(vm["datatypefile"].as<std::string>().c_str());

      if(!dtofs) {
        std::cerr << "Could not open data type definition file " << vm["datatypefile"].as<std::string>() << "." << std::endl;
        exit(-1);
      }

      hexabus::DTypeFileGenerator dtg;
      dtg(tableBuilder.get_endpoint_table(), dtofs);
      dtofs.close();
    }

  } else {
    return 1;
  }

  return 0;
}
