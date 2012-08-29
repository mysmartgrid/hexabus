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
  module_instantiation(module_table_ptr mt, hbc_doc& hbc) : _m(mt), _hbc(hbc) { }

  void operator()(instantiation_doc& inst) const {
    std::cout << "Instantiating module " << inst.moduleclass << std::endl;
    module_table::iterator mod = _m->find(inst.moduleclass);
    if(mod == _m->end()) {
      std::ostringstream oss;
      oss << "[" << inst.read_from_file << ":" << inst.lineno << "] Can not instantiate module \"" << inst.moduleclass << "\" - module does not exist." << std::endl;
      throw ModuleNotFoundException(oss.str());
    }
  }

  void operator()(endpoint_doc& ep) const { }
  void operator()(alias_doc& alias) const { }
  void operator()(statemachine_doc& statemachine) const { }
  void operator()(include_doc& include) const { }
  void operator()(module_doc& module) const { }

  module_table_ptr _m;
  hbc_doc _hbc;
};

void ModuleInstantiation::operator()(hbc_doc& hbc) {
  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    boost::apply_visitor(module_table_builder(_m), block);
  }

  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    boost::apply_visitor(module_instantiation(_m, hbc), block);
  }
}

void ModuleInstantiation::print_module_table() {
  std::cout << "Module table:" << std::endl;
  for(module_table::iterator it = _m->begin(); it != _m->end(); it++) {
    std::cout << it->first << ": Module named " << it->second.name << " defined in " << it->second.read_from_file << " line " << it->second.lineno << std::endl;
  }
}

