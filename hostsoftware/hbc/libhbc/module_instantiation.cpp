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

  void operator()(condition_doc& cond, condition_doc& inst_cond, placeholder_list_doc& placeholders, std::vector<inst_parameter_doc>& parameters) {
    module_instantiation m(_m, _hbc);
    atomic_condition_doc inst_at_cond;
    compound_condition_doc inst_comp_cond;
    switch(cond.which()) {
      case 0: // atomic_condition
        m(boost::get<atomic_condition_doc>(cond).geid, inst_at_cond.geid, placeholders, parameters);
        inst_at_cond.comp_op = boost::get<atomic_condition_doc>(cond).comp_op;
        m(boost::get<atomic_condition_doc>(cond).constant, inst_at_cond.constant, placeholders, parameters);
        inst_cond = inst_at_cond;
        break;
      case 1: // compound_condition
        m(boost::get<compound_condition_doc>(cond).condition_a, inst_comp_cond.condition_a, placeholders, parameters);
        inst_comp_cond.bool_op = boost::get<compound_condition_doc>(cond).bool_op;
        m(boost::get<compound_condition_doc>(cond).condition_b, inst_comp_cond.condition_b, placeholders, parameters);
        inst_cond = inst_comp_cond;
        break;

      default:
        // do nothing.
        break;
    }
  }

  void operator()(global_endpoint_id_doc& geid, global_endpoint_id_doc& inst_geid, placeholder_list_doc& placeholders, std::vector<inst_parameter_doc>& parameters) {
    int placeholder_index = -1;
    switch(geid.device_alias.which()) {
      case 0: // string
        inst_geid.device_alias = boost::get<std::string>(geid.device_alias);
        break;
      case 1: // placeholder
        // find index of placeholder
        for(int i = 0; i < placeholders.placeholders.size(); i++) {
          if(placeholders.placeholders[i].name == boost::get<placeholder_doc>(geid.device_alias).name)
            placeholder_index = i;
        }
        if(placeholder_index == -1) {
          std::ostringstream oss;
          oss << "Placeholder not found: " << boost::get<placeholder_doc>(geid.device_alias).name << "." << std::endl;
          throw InvalidPlaceholderException(oss.str());
        }

        // get corresponding element from instance parameter list and put it in the geid struct
        try {
          inst_geid.device_alias = boost::get<std::string>(parameters[placeholder_index]);
        } catch (boost::bad_get e) {
          std::ostringstream oss;
          oss << boost::get<placeholder_doc>(geid.device_alias).name << ": Invalid parameter type (expected device name)" << std::endl;
          throw InvalidParameterTypeException(oss.str());
        }
        break;

      default:
        break;
    }

    placeholder_index = -1;
    switch(geid.endpoint_name.which()) {
      case 0: // string
        inst_geid.endpoint_name = boost::get<std::string>(geid.endpoint_name);
        break;
      case 1: // placeholder
        // find placeholder in list
        for(int i = 0; i < placeholders.placeholders.size(); i++) {
          if(placeholders.placeholders[i].name == boost::get<placeholder_doc>(geid.endpoint_name).name)
            placeholder_index = i;
        }
        if(placeholder_index == -1) {
          std::ostringstream oss;
          oss << "Placeholder not found: " << boost::get<placeholder_doc>(geid.endpoint_name).name << "." << std::endl;
          throw InvalidPlaceholderException(oss.str());
        }

        // put element from parameter list into geid struct
        try {
          inst_geid.endpoint_name = boost::get<std::string>(parameters[placeholder_index]);
        } catch(boost::bad_get e) {
          std::ostringstream oss;
          oss << boost::get<placeholder_doc>(geid.endpoint_name).name << ": Invalid parameter type (expected device name)" << std::endl;
          throw InvalidParameterTypeException(oss.str());
        }
        break;

      default:
        break;
    }
  }

  void operator()(constant_doc& constant, constant_doc& inst_constant, placeholder_list_doc& placeholders, std::vector<inst_parameter_doc>& parameters) {
    switch(constant.which()) {
      case 0: // placeholder_doc
        {
          // find placeholder in list
          int placeholder_index = -1;
          for(int i = 0; i < placeholders.placeholders.size(); i++) {
            if(placeholders.placeholders[i].name == boost::get<placeholder_doc>(constant).name)
              placeholder_index = i;
          }
          if(placeholder_index == -1) {
            std::ostringstream oss;
            oss << "Placeholder not found: " << boost::get<placeholder_doc>(constant).name << "." << std::endl;
            throw InvalidPlaceholderException(oss.str());
          }

          // put element from parameter list into constant
          try {
            inst_constant = boost::get<constant_doc>(parameters[placeholder_index]);
          } catch (boost::bad_get e) {
            std::ostringstream oss;
            oss << boost::get<placeholder_doc>(constant).name << ": Invalid parameter type (expected constant)" << std::endl;
            throw InvalidParameterTypeException(oss.str());
          }
        }
        break;
      case 1: // unsigned int
      case 2: // float
        inst_constant = constant;
        break;

      default:
        // do nothing
        break;
    }
  }

  void operator()(command_doc& command, command_doc& inst_command, placeholder_list_doc& placeholders, std::vector<inst_parameter_doc>& parameters) {
    module_instantiation m(_m, _hbc);
    inst_command.write_command.lineno = command.write_command.lineno;
    m(command.write_command.geid, inst_command.write_command.geid, placeholders, parameters);
    m(command.write_command.constant, inst_command.write_command.constant, placeholders, parameters);
  }

  void operator()(command_block_doc& commands, command_block_doc& inst_commands, placeholder_list_doc& placeholders, std::vector<inst_parameter_doc>& parameters) {
    module_instantiation m(_m, _hbc);
    BOOST_FOREACH(command_doc command, commands.commands) {
      command_doc inst_command;
      m(command, inst_command, placeholders, parameters);
      inst_commands.commands.push_back(inst_command);
    }
    inst_commands.goto_command.lineno = commands.goto_command.lineno;
    inst_commands.goto_command.target_state = commands.goto_command.target_state;
  }

  void operator()(guarded_command_block_doc& guarded_command_block, guarded_command_block_doc& inst_guarded_command_block, placeholder_list_doc& placeholders, std::vector<inst_parameter_doc>& parameters) {
    module_instantiation m(_m, _hbc);
    m(guarded_command_block.condition, inst_guarded_command_block.condition, placeholders, parameters);
    m(guarded_command_block.command_block, inst_guarded_command_block.command_block, placeholders, parameters);
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
        m(if_clause.if_block, inst_if_clause.if_block, mod->second.placeholderlist, inst.parameters);

        // else if blocks
        BOOST_FOREACH(guarded_command_block_doc guarded_command_block, if_clause.else_if_blocks) {
          guarded_command_block_doc inst_guarded_command_block;
          m(guarded_command_block, inst_guarded_command_block, mod->second.placeholderlist, inst.parameters);
          inst_if_clause.else_if_blocks.push_back(inst_guarded_command_block);
        }

        // else block
        if(if_clause.else_clause.present) {
          inst_if_clause.else_clause.present = 1;
          m(if_clause.else_clause.commands, inst_if_clause.else_clause.commands, mod->second.placeholderlist, inst.parameters);
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

