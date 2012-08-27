#include "filename_annotation.hpp"

using namespace hexabus;

struct annotate_struct : boost::static_visitor<> {
  annotate_struct(std::string filename) : _f(filename) { }

  void operator()(endpoint_doc& ep) const {
    if(ep.read_from_file == "") {
      ep.read_from_file = _f;
    }
  }

  void operator()(statemachine_doc& statemachine) const {
    if(statemachine.read_from_file == "")
      statemachine.read_from_file = _f;
  }

  void operator()(include_doc& include) const {
    if(include.read_from_file == "")
      include.read_from_file = _f;
  }

  void operator()(module_doc& module) const {
    if(module.read_from_file == "")
      module.read_from_file = _f;
  }

  void operator()(alias_doc& alias) const {
    if(alias.read_from_file == "")
      alias.read_from_file = _f;
  }

  std::string _f;
};

void FilenameAnnotation::operator()(hbc_doc& hbc) {
  for(std::vector<hbc_block>::iterator it = hbc.blocks.begin(); it != hbc.blocks.end(); it++) {
    boost::apply_visitor(annotate_struct(_f), *it);
  }
}
