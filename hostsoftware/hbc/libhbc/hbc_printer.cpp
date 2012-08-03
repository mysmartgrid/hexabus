#include "hbc_printer.hpp"
#include <libhbc/hbc_enums.hpp>
#include <boost/foreach.hpp>

using namespace hexabus;

void hexabus::tab(int indent) {
  for (int i = 0; i < indent; i++)
    std::cout << "⎜  ";
}

struct hbc_node_printer : boost::static_visitor<> {
  hbc_node_printer(int indent = 0)
    : indent(indent)
  { }

  void operator()(include_doc const& include) const {
    std::cout << "[" << include.lineno << "] include " << include.filename << std::endl;
  }

  void operator()(datatype_doc const& datatype) const {
    // TODO
    std::cout << "(datatype)";
  }

  void operator()(access_level_doc const& access_level) const {
    // TODO
    std::cout << "(access_level)";
  }

  void operator()(float const& f) const {
    std::cout << f;
  }

  void operator()(placeholder_doc const& placeholder) const {
    std::cout << "<" << placeholder.name << ">";
  }

  void operator()(ep_name_doc const& ep_name) const {
    std::cout << "name: " << ep_name.name;
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
    tab(indent);
    std::cout << "[" << endpoint.lineno << "] endpoint " << endpoint.eid << std::endl;
    tab(indent);
    std::cout << "⎛" << std::endl;
    BOOST_FOREACH(endpoint_cmd_doc endpoint_cmd, endpoint.cmds) {
      tab(indent+1);
      boost::apply_visitor(hbc_node_printer(indent+1), endpoint_cmd);
      std::cout << std::endl;
    }
    std::cout << "⎝" << std::endl;
  }

  void operator()(eid_list_doc const& eid_list) const {
    std::cout << "eids: ";
    BOOST_FOREACH(unsigned int eid, eid_list.eids) {
      std::cout << eid << " ";
    }
  }

  void operator()(alias_ip_doc const& alias_ip) const {
    std::cout << "address: " << alias_ip.ipv6_address;
  }

  void operator()(alias_eids_doc const& alias_eids) const {
    hbc_node_printer p;
    p(alias_eids.eid_list);
  }

  void operator()(alias_doc const& alias) const {
    tab(indent);
    std::cout << "[" << alias.lineno << "] alias <" << alias.device_name << ">" << std::endl;
    tab(indent);
    std::cout << "⎛" << std::endl;
    BOOST_FOREACH(alias_cmd_doc cmd, alias.cmds) {
      tab(indent+1);
      boost::apply_visitor(hbc_node_printer(indent+1), cmd);
      std::cout << std::endl;
    }
    std::cout << "⎝" << std::endl;
  }

  void operator()(global_endpoint_id_doc const& global_endpoint_id) const {
    hbc_printer p;
    boost::apply_visitor(hbc_node_printer(indent), global_endpoint_id.device_alias);
    std::cout << ".";
    boost::apply_visitor(hbc_node_printer(indent), global_endpoint_id.endpoint_name);
  }

  void operator()(atomic_condition_doc const& atomic_condition) const {
    hbc_node_printer p;
    p(atomic_condition.geid);
    switch(atomic_condition.comp_op) {
      case STM_EQ:
        std::cout << " == "; break;
      case STM_LEQ:
        std::cout << " <= "; break;
      case STM_GEQ:
        std::cout << " >= "; break;
      case STM_LT:
        std::cout << " < "; break;
      case STM_GT:
        std::cout << " > "; break;
      case STM_NEQ:
        std::cout << " != "; break;
      default:
        std::cout << " (op not implemented?!) "; break;
    }
    boost::apply_visitor(hbc_node_printer(indent), atomic_condition.constant);
  }

  void operator()(compound_condition_doc const& compound_condition, unsigned int ind = 0) const {
    hbc_node_printer p;
    std::cout << " ( ";
    boost::apply_visitor(hbc_node_printer(indent), compound_condition.condition_a);
    switch(compound_condition.bool_op) {
      case OR:
        std::cout << " || "; break;
      case AND:
        std::cout << " && "; break;
      default:
        std::cout << " (operator not implemented?!) ";
    }
    boost::apply_visitor(hbc_node_printer(indent), compound_condition.condition_b);
    std::cout << " ) ";
  }

  void operator()(write_command_doc const& write_command, unsigned int ind = 0) const {
    hbc_node_printer p;
    tab(ind);
    std::cout << "[" << write_command.lineno << "] write ";
    p(write_command.geid);
    std::cout << " := ";
    boost::apply_visitor(hbc_node_printer(indent), write_command.constant);
    std::cout << std::endl;
  }

  void operator()(command_doc const& command, unsigned int ind = 0) const {
    hbc_node_printer p;
    p(command.write_command, ind);
  }

  void operator()(goto_command_doc const& goto_command, unsigned int ind = 0) const {
    tab(ind);
    std::cout << "[" << goto_command.lineno << "] goto " << goto_command.target_state << std::endl;
  }

  void operator()(command_block_doc const& command_block, unsigned int ind = 0) const {
    hbc_node_printer p;
    BOOST_FOREACH(command_doc command, command_block.commands) {
      p(command, ind);
    }
    p(command_block.goto_command, ind);
  }

  void operator()(guarded_command_block_doc const& guarded_command_block, unsigned int ind) const {
    hbc_node_printer p;
    std::cout << "(";
    boost::apply_visitor(hbc_node_printer(ind), guarded_command_block.condition);
    std::cout << ")" << std::endl;
    tab(ind);
    std::cout << "⎛" << std::endl;
    p(guarded_command_block.command_block, ind+1);
    tab(ind);
    std::cout << "⎝" << std::endl;
  }

  void operator()(if_clause_doc const& if_clause, unsigned int ind = 0) const {
    hbc_node_printer p;
    tab(ind);
    std::cout << "[" << if_clause.lineno << "] if ";
    p(if_clause.if_block, ind);
    BOOST_FOREACH(guarded_command_block_doc else_if_block, if_clause.else_if_blocks) {
      tab(ind);
      std::cout << "else if ";
      p(else_if_block, ind);
    }
    tab(ind);
    std::cout << "else" << std::endl; // TODO only print this if there is one. Find a reliable way to find out whether there is one.
    tab(ind);
    std::cout << "⎛" << std::endl;
    p(if_clause.else_block, ind+1);
    tab(ind);
    std::cout << "⎝" << std::endl;
  }

  void operator()(in_clause_doc const& in_clause, unsigned int ind = 0) const {
    hbc_node_printer p;
    tab(ind);
    std::cout << "[" << in_clause.lineno << "] in (" << in_clause.name << ")" << std::endl;
    tab(ind);
    std::cout << "⎛" << std::endl;
    BOOST_FOREACH(if_clause_doc if_clause, in_clause.if_clauses) {
      p(if_clause, ind+1);
    }
    tab(ind);
    std::cout << "⎝" << std::endl;
  }

  void operator()(stateset_doc const& stateset) const {
    tab(indent);
    std::cout << "state set: ";
    BOOST_FOREACH(std::string state_name, stateset.states) {
      std::cout << "<" << state_name << "> ";
    }
    std::cout << std::endl;
  }

  void operator()(statemachine_doc const& statemachine) const {
    hbc_node_printer p;
    tab(indent);
    std::cout << "statemachine " << statemachine.name << std::endl;
    tab(indent);
    std::cout << "⎛" << std::endl;
    tab(indent+1);
    p(statemachine.stateset);
    BOOST_FOREACH(in_clause_doc in_clause, statemachine.in_clauses) {
      p(in_clause, indent+1);
    }
    std::cout << "⎝" << std::endl;
  }

  void operator()(std::string const& s) const {
    std::cout << s;
  }

  void operator()(placeholder_list_doc const& placeholder_list) const {
    hbc_node_printer p;
    tab(indent);
    std::cout << "placeholders: ";
    BOOST_FOREACH(placeholder_doc placeholder, placeholder_list.placeholders) {
      p(placeholder);
    }
  }

  void operator()(module_doc const& module) const {
    hbc_node_printer p;
    tab(indent);
    std::cout << "[" << module.lineno << "] module <" << module.name << ">" << std::endl;
    tab(indent);
    std::cout << "⎛" << std::endl;
    tab(indent + 1);
    p(module.placeholderlist);
    std::cout << std::endl;
    tab(indent + 1);
    p(module.stateset);
    BOOST_FOREACH(in_clause_doc in_clause, module.in_clauses) {
      p(in_clause, indent+1);
    }
    tab(indent);
    std::cout << "⎝" << std::endl;
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
    std::cout << std::endl;
  }
}

