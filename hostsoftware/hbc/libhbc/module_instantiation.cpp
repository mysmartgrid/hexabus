#include "module_instantiation.hpp"

#include <libhbc/error.hpp>

using namespace hexabus;

struct module_table_builder : boost::static_visitor<> { // TODO this could be moved to table_generator..!
  module_table_builder(module_table_ptr mt) : _m(mt) { }

  void operator()(module_doc& module) const {
    // check if module name is still available
    for(module_table::iterator it = _m->begin(); it != _m->end(); it++) {
      if(it->first == module.name) {
        std::ostringstream oss;
        oss << "[" << module.read_from_file << ":" << module.lineno << "] Duplicate module name." << std::endl;
        throw DuplicateEntryException(oss.str());
      }
    }

    _m->insert(std::pair<std::string, module_doc>(module.name, module));
  }

  void operator()(endpoint_doc& ep) const { }
  void operator()(alias_doc& alias) const { }
  void operator()(statemachine_doc& statemachine) const { }
  void operator()(include_doc& include) const { }
  void operator()(instantiation_doc& inst) const { }

  module_table_ptr _m;
};

struct module_instantiation : boost::static_visitor<> {
  module_instantiation(module_table_ptr mt) : _m(mt) { }

  void operator()(instantiation_doc& inst) const { }

  void operator()(endpoint_doc& ep) const { }
  void operator()(alias_doc& alias) const { }
  void operator()(statemachine_doc& statemachine) const { }
  void operator()(include_doc& include) const { }
  void operator()(module_doc& module) const { }

  module_table_ptr _m;
};

void ModuleInstantiation::operator()(hbc_doc& hbc) {
  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    boost::apply_visitor(module_table_builder(_m), block);
  }

  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    boost::apply_visitor(module_instantiation(_m), block);
  }
}

void ModuleInstantiation::print_module_table() {
  for(module_table::iterator it = _m->begin(); it != _m->end(); it++) {
    std::cout << it->first << ": Module named " << it->second.name << " defined in " << it->second.read_from_file << " line " << it->second.lineno << std::endl;
  }
}

