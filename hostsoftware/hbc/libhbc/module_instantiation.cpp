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

  void operator()(condition_doc& cond, condition_doc& inst_cond) {
    // TODO
  }

  void operator()(command_block_doc& commands, command_block_doc& inst_commands) {
    // TODO
  }

  void operator()(guarded_command_block_doc& guarded_command_block, guarded_command_block_doc& inst_guarded_command_block) {
    // TODO
  }

  void operator()(instantiation_doc& inst) const {
    // find module class - throw error if it's not there
    std::cout << "Instantiating module " << inst.moduleclass << std::endl;
    module_table::iterator mod = _m->find(inst.moduleclass);
    if(mod == _m->end()) {
      std::ostringstream oss;
      oss << "[" << inst.read_from_file << ":" << inst.lineno << "] Can not instantiate module \"" << inst.moduleclass << "\" - module does not exist." << std::endl;
      throw ModuleNotFoundException(oss.str());
    }

    // build module instance
    // TODO check if parameter list and placeholder list are same length
    statemachine_doc instance;
    instance.lineno = inst.lineno;
    std::ostringstream inst_name_oss;
    inst_name_oss << "inst_" << inst.name << ":" << inst.moduleclass;
    instance.name = inst_name_oss.str();
    instance.stateset = mod->second.stateset;

    BOOST_FOREACH(in_clause_doc in_clause, mod->second.in_clauses) {
      in_clause_doc inst_in_clause;
      inst_in_clause.lineno = in_clause.lineno;
      inst_in_clause.name = in_clause.name;

      module_instantiation m(_m, _hbc);
      BOOST_FOREACH(if_clause_doc if_clause, in_clause.if_clauses) {
        if_clause_doc inst_if_clause;
        inst_if_clause.lineno = if_clause.lineno;

        // if block
        m(if_clause.if_block, inst_if_clause.if_block);

        // else if blocks
        BOOST_FOREACH(guarded_command_block_doc guarded_command_block, if_clause.else_if_blocks) {
          guarded_command_block_doc inst_guarded_command_block;
          m(guarded_command_block, inst_guarded_command_block);
          inst_if_clause.else_if_blocks.push_back(inst_guarded_command_block);
        }

        // else block
        if(if_clause.else_clause.present) {
          inst_if_clause.else_clause.present = 1;
          m(if_clause.else_clause.commands, inst_if_clause.else_clause.commands);
        }
      }

      instance.in_clauses.push_back(inst_in_clause);
    }

    // TODO to replace a placeholder by its parameter:
    // - find placeholder in placeholder list
    //   - exc if missing
    // - take its index, look in parameter list
    //   - exc if it's missing (should not happen, because we will check list length beforehand)
    // - check if parameter is right type (literal number, device, endpoint)
    //   - exc if it isn't
    // - put actual value in instance
  }

  void operator()(endpoint_doc& ep) const { }
  void operator()(alias_doc& alias) const { }
  void operator()(statemachine_doc& statemachine) const { }
  void operator()(include_doc& include) const { }
  void operator()(module_doc& module) const { }

  module_table_ptr _m;
  hbc_doc& _hbc;
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

