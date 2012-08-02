#include "hbc_printer.hpp"
#include <boost/foreach.hpp>

using namespace hexabus;

void hexabus::tab(int indent) {
  for (int i = 0; i < indent; i++)
    std::cout << ' ';
}

struct hbc_node_printer : boost::static_visitor<> {
  hbc_node_printer(int indent = 0)
    : indent(indent)
  { }

  void operator()(include_doc const& include) const {
    std::cout << "Line " << include.lineno << " - Include " << include.filename << std::endl;
  }

  void operator()(datatype_doc const& datatype) const {
    // TODO
    std::cout << "(datatype coming soon!)" << std::endl;
  }

  void operator()(access_level_doc const& access_level) const {
    // TODO
    std::cout << "(access_level coming soon!)" << std::endl;
  }

  void operator()(float const& f) const {
    std::cout << "Float constant: " << f << std::endl;
  }

  void operator()(placeholder_doc const& placeholder) const {
    std::cout << "Placeholder: \"" << placeholder.name << "\""<< std::endl;
  }

  void operator()(ep_name_doc const& ep_name) const {
    std::cout << "EP Name: " << ep_name.name << std::endl;
  }

  void operator()(ep_datatype_doc const& ep_datatype) const {
    hbc_node_printer p;
    p(ep_datatype.datatype);
  }

  void operator()(ep_access_doc const& ep_access) const {
    BOOST_FOREACH(access_level_doc const& access_level, ep_access.access_levels) {
      hbc_node_printer p;
      p(access_level);
    }
  }

  void operator()(endpoint_doc const& endpoint) const {
    std::cout << "Line " << endpoint.lineno << ": Endpoint definition." << std::endl
      << "EID " << endpoint.eid << std::endl;
    BOOST_FOREACH(endpoint_cmd_doc endpoint_cmd, endpoint.cmds) {
      boost::apply_visitor(hbc_node_printer(indent), endpoint_cmd);
    }
  }

  void operator()(eid_list_doc const& eid_list) const {
    std::cout << "Eid list: ";
    BOOST_FOREACH(unsigned int eid, eid_list.eids) {
      std::cout << eid << " ";
    }
    std::cout << std::endl;
  }

  void operator()(alias_ip_doc const& alias_ip) const {
    std::cout << "IPv6 address: " << alias_ip.ipv6_address << std::endl;
  }

  void operator()(alias_eids_doc const& alias_eids) const {
    hbc_node_printer p;
    p(alias_eids.eid_list);
  }

  void operator()(alias_doc const& alias) const {
    std::cout << "Alias definition in line " << alias.lineno << " - Name: " << alias.device_name << std::endl;
    BOOST_FOREACH(alias_cmd_doc cmd, alias.cmds) {
      boost::apply_visitor(hbc_node_printer(indent), cmd);
    }
  }

  void operator()(global_endpoint_id_doc const& global_endpoint_id) const {
    hbc_printer p;
    std::cout << "Global Endpoint ID: Device: ";
    boost::apply_visitor(hbc_node_printer(indent), global_endpoint_id.device_alias);
    std::cout << " Endpoint: ";
    boost::apply_visitor(hbc_node_printer(indent), global_endpoint_id.endpoint_name);
    std::cout << std::endl;
  }

  void operator()(atomic_condition_doc const& atomic_condition) const {
    hbc_node_printer p;
    std::cout << "Atomic Condition (";
    p(atomic_condition.geid);
    std::cout << "Operator: " << atomic_condition.comp_op << " ";
    boost::apply_visitor(hbc_node_printer(indent), atomic_condition.constant);
  }

  void operator()(compound_condition_doc const& compound_condition) const {
    hbc_node_printer p;
    std::cout << "Compound Condition. Component A:" << std::endl;
    boost::apply_visitor(hbc_node_printer(indent), compound_condition.condition_a);
    std::cout << "(operator coming soon!)" << std::endl; // TODO!
    std::cout << "Component B:" << std::endl;
    boost::apply_visitor(hbc_node_printer(indent), compound_condition.condition_b);
  }

  void operator()(write_command_doc const& write_command) const {
    hbc_node_printer p;
    std::cout << "Write Command in line " << write_command.lineno << std::endl;
    p(write_command.geid);
    boost::apply_visitor(hbc_node_printer(indent), write_command.constant);
  }

  void operator()(command_doc const& command) const {
    hbc_node_printer p;
    p(command.write_command);
  }

  void operator()(goto_command_doc const& goto_command) const {
    std::cout << "Goto command in line " << goto_command.lineno
      << "Target state: " << goto_command.target_state << std::endl;
  }

  void operator()(command_block_doc const& command_block) const {
    hbc_node_printer p;
    BOOST_FOREACH(command_doc command, command_block.commands) {
      p(command);
    }
    p(command_block.goto_command);
  }

  void operator()(guarded_command_block_doc const& guarded_command_block) const {
    hbc_node_printer p;
    std::cout << "Guarded command block. Condition:" << std::endl;
    boost::apply_visitor(hbc_node_printer(indent), guarded_command_block.condition);
    std::cout << "Commands:" << std::endl;
    p(guarded_command_block.command_block);
    std::cout << "Command block end." << std::endl;
  }

  void operator()(if_clause_doc const& if_clause) const {
    hbc_node_printer p;
    std::cout << "If clause in line " << if_clause.lineno << " ";
    std::cout << "If-block:" << std::endl;
    p(if_clause.if_block);
    BOOST_FOREACH(guarded_command_block_doc else_if_block, if_clause.else_if_blocks) {
      std::cout << "Else-if-Block:" << std::endl;
      p(else_if_block);
    }
    std::cout << "Else-Block:" << std::endl;
    p(if_clause.else_block);
  }

  void operator()(in_clause_doc const& in_clause) const {
    hbc_node_printer p;
    std::cout << "In-clause in line " << in_clause.lineno << ": "
      << in_clause.name << std::endl;
    std::cout << "If-clauses:" << std::endl;
    BOOST_FOREACH(if_clause_doc if_clause, in_clause.if_clauses) {
      p(if_clause);
    }
    std::cout << "In clause end." << std::endl;
  }

  void operator()(stateset_doc const& stateset) const {
    std::cout << "State set: ";
    BOOST_FOREACH(std::string state_name, stateset.states) {
      std::cout << state_name << " ";
    }
    std::cout << std::endl;
  }

  void operator()(statemachine_doc const& statemachine) const {
    hbc_node_printer p;
    std::cout << "State machine: Name: " << statemachine.name << std::endl;
    p(statemachine.stateset);
    BOOST_FOREACH(in_clause_doc in_clause, statemachine.in_clauses) {
      p(in_clause);
    }
    std::cout << "State machine end." << std::endl;
  }

  void operator()(std::string const& s) const {
    std::cout << s;
  }

  void operator()(placeholder_list_doc const& placeholder_list) const {
    hbc_node_printer p;
    std::cout << "Placeholder list: " << std::endl;
    BOOST_FOREACH(placeholder_doc placeholder, placeholder_list.placeholders) {
      p(placeholder);
    }
    std::cout << "Placeholder list end." << std::endl;
  }

  void operator()(module_doc const& module) const {
    hbc_node_printer p;
    std::cout << "Module definition in line " << module.lineno << ":"
      << "Name: " << module.name << std::endl;
    p(module.placeholderlist);
    p(module.stateset);
    BOOST_FOREACH(in_clause_doc in_clause, module.in_clauses) {
      p(in_clause);
    }
    std::cout << "Module definition end." << std::endl;
  }

  void operator()(instantiation_doc const& instantiation) const {
    hbc_node_printer p;
    std::cout << "Module instantiation in line " << instantiation.lineno
      << " Name: " << instantiation.name << " Module class: "
      << instantiation.moduleclass << std::endl;
    std::cout << "Parameters" << std::endl;
    BOOST_FOREACH(inst_parameter_doc inst_parameter, instantiation.parameters) { // TODO this code "works"
      if(inst_parameter.which() == 0)
        boost::apply_visitor(hbc_node_printer(indent), boost::get<constant_doc>(inst_parameter));
      else if(inst_parameter.which() == 1)
        p(boost::get<float>(inst_parameter));
    }
  }

  void operator()(hbc_block const& block) const {
    hbc_node_printer p;
    std::cout << "[Error: Block Type not defined]" << std::endl;
  }

  int indent;
};

void hbc_printer::operator()(hbc_doc const& hbc) const
{
  // TODO a lot of pretty printing!
  BOOST_FOREACH(hbc_block const& block, hbc.blocks)
  {
    boost::apply_visitor(hbc_node_printer(indent), block);
  }
}

