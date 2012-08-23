#include "hbc_printer.hpp"
#include <libhbc/hbc_enums.hpp>
#include <boost/foreach.hpp>

using namespace hexabus;

void hexabus::tab(int indent) {
  for (int i = 0; i < indent; i++)
    std::cout << "⎜  ";
}

struct hbc_node_printer : boost::static_visitor<> {
  hbc_node_printer(int indent = 0, std::ostream& ostr = (std::cout))
    : indent(indent), ostr(ostr)
  { }

  void operator()(include_doc const& include) const {
    ostr << "[" << include.lineno << "] include " << include.filename << std::endl;
  }

  void operator()(float const& f) const {
      ostr << f;
  }

  void operator()(unsigned int const& i) const {
      ostr << i;
  }

  void operator()(placeholder_doc const& placeholder) const {
      ostr << "<" << placeholder.name << ">";
  }

  void operator()(ep_eid_doc const& ep_eid) const {
    ostr << "eid: " << ep_eid.eid;
  }

  void operator()(ep_datatype_doc const& ep_datatype) const {
    ostr << "datatype: ";
    switch(ep_datatype.dtype) {
      case DT_BOOL:
        ostr << "BOOL"; break;
      case DT_UINT8:
        ostr << "UINT8"; break;
      case DT_UINT32:
        ostr << "UINT32"; break;
      case DT_FLOAT:
        ostr << "FLOAT"; break;
      default:
        ostr << "(not implemented?)"; break;
    }
  }

  void operator()(ep_access_doc const& ep_access) const {
    ostr << "access: ";
    BOOST_FOREACH(access_level const& access_level, ep_access.access_levels) {
      switch(access_level) {
        case AC_READ:
          ostr << "read"; break;
        case AC_WRITE:
          ostr << "write"; break;
        case AC_BROADCAST:
          ostr << "broadcast"; break;
        default:
          ostr << "(acces level not implemented?)";
      }
      ostr << " ";
    }
  }

  void operator()(endpoint_doc const& endpoint) const {
    tab(indent);
    ostr << "[" << endpoint.lineno << "] endpoint " << endpoint.name << std::endl;
    tab(indent);
    ostr << "⎛" << std::endl;
    BOOST_FOREACH(endpoint_cmd_doc endpoint_cmd, endpoint.cmds) {
      tab(indent+1);
      boost::apply_visitor(hbc_node_printer(indent+1, ostr), endpoint_cmd);
      ostr << std::endl;
    }
    ostr << "⎝" << std::endl;
  }

  void operator()(eid_list_doc const& eid_list) const {
    ostr << "eids: ";
    BOOST_FOREACH(unsigned int eid, eid_list.eids) {
      ostr << eid << " ";
    }
  }

  void operator()(alias_ip_doc const& alias_ip) const {
    ostr << "address: " << alias_ip.ipv6_address;
  }

  void operator()(alias_eids_doc const& alias_eids) const {
    hbc_node_printer p;
    p(alias_eids.eid_list);
  }

  void operator()(alias_doc const& alias) const {
    tab(indent);
    ostr << "[" << alias.lineno << "] alias <" << alias.device_name << ">" << std::endl;
    tab(indent);
    ostr << "⎛" << std::endl;
    BOOST_FOREACH(alias_cmd_doc cmd, alias.cmds) {
      tab(indent+1);
      boost::apply_visitor(hbc_node_printer(indent+1, ostr), cmd);
      ostr << std::endl;
    }
    ostr << "⎝" << std::endl;
  }

  void operator()(global_endpoint_id_doc const& global_endpoint_id, std::ostream& ostr = (std::cout)) const {
    hbc_printer p;
    boost::apply_visitor(hbc_node_printer(indent, ostr), global_endpoint_id.device_alias);
    ostr << ".";
    boost::apply_visitor(hbc_node_printer(indent, ostr), global_endpoint_id.endpoint_name);
  }

  void operator()(atomic_condition_doc const& atomic_condition) const {
    hbc_node_printer p;
    p(atomic_condition.geid, ostr);
    switch(atomic_condition.comp_op) {
      case STM_EQ:
        ostr << " == "; break;
      case STM_LEQ:
        ostr << " <= "; break;
      case STM_GEQ:
        ostr << " >= "; break;
      case STM_LT:
        ostr << " < "; break;
      case STM_GT:
        ostr << " > "; break;
      case STM_NEQ:
        ostr << " != "; break;
      default:
        ostr << " (op not implemented?!) "; break;
    }
    boost::apply_visitor(hbc_node_printer(indent, ostr), atomic_condition.constant);
  }

  void operator()(compound_condition_doc const& compound_condition, unsigned int ind = 0) const {
    ostr << " ( ";
    boost::apply_visitor(hbc_node_printer(indent, ostr), compound_condition.condition_a);
    switch(compound_condition.bool_op) {
      case OR:
        ostr << " || "; break;
      case AND:
        ostr << " && "; break;
      default:
        ostr << " (operator not implemented?!) ";
    }
    boost::apply_visitor(hbc_node_printer(indent, ostr), compound_condition.condition_b);
    ostr << " ) ";
  }

  void operator()(write_command_doc const& write_command, unsigned int ind = 0) const {
    hbc_node_printer p(ind, ostr);
    tab(ind);
    ostr << "[" << write_command.lineno << "] write ";
    p(write_command.geid, ostr);
    ostr << " := ";
    boost::apply_visitor(hbc_node_printer(indent, ostr), write_command.constant);
    ostr << std::endl;
  }

  void operator()(command_doc const& command, unsigned int ind = 0) const {
    hbc_node_printer p(ind, ostr);
    p(command.write_command, ind);
  }

  void operator()(goto_command_doc const& goto_command, unsigned int ind = 0) const {
    tab(ind);
    ostr << "[" << goto_command.lineno << "] goto " << goto_command.target_state << std::endl;
  }

  void operator()(command_block_doc const& command_block, unsigned int ind = 0) const {
    hbc_node_printer p(ind, ostr);
    BOOST_FOREACH(command_doc command, command_block.commands) {
      p(command, ind);
    }
    p(command_block.goto_command, ind);
  }

  void operator()(guarded_command_block_doc const& guarded_command_block, unsigned int ind) const {
    hbc_node_printer p;
    ostr << "(";
    boost::apply_visitor(hbc_node_printer(ind), guarded_command_block.condition);
    ostr << ")" << std::endl;
    tab(ind);
    ostr << "⎛" << std::endl;
    p(guarded_command_block.command_block, ind+1);
    tab(ind);
    ostr << "⎝" << std::endl;
  }

  void operator()(if_clause_doc const& if_clause, unsigned int ind = 0) const {
    hbc_node_printer p;
    tab(ind);
    ostr << "[" << if_clause.lineno << "] if ";
    p(if_clause.if_block, ind);
    BOOST_FOREACH(guarded_command_block_doc else_if_block, if_clause.else_if_blocks) {
      tab(ind);
      ostr << "else if ";
      p(else_if_block, ind);
    }
    if(if_clause.else_clause.present == 1) {
      tab(ind);
      ostr << "else" << std::endl;
      tab(ind);
      ostr << "⎛" << std::endl;
      p(if_clause.else_clause.commands, ind+1);
      tab(ind);
      ostr << "⎝" << std::endl;
    }
  }

  void operator()(in_clause_doc const& in_clause, unsigned int ind = 0) const {
    hbc_node_printer p;
    tab(ind);
    ostr << "[" << in_clause.lineno << "] in (" << in_clause.name << ")" << std::endl;
    tab(ind);
    ostr << "⎛" << std::endl;
    BOOST_FOREACH(if_clause_doc if_clause, in_clause.if_clauses) {
      p(if_clause, ind+1);
    }
    tab(ind);
    ostr << "⎝" << std::endl;
  }

  void operator()(stateset_doc const& stateset) const {
    tab(indent);
    ostr << "state set: ";
    BOOST_FOREACH(std::string state_name, stateset.states) {
      ostr << "<" << state_name << "> ";
    }
    ostr << std::endl;
  }

  void operator()(statemachine_doc const& statemachine) const {
    hbc_node_printer p;
    tab(indent);
    ostr << "statemachine <" << statemachine.name << ">" << std::endl;
    tab(indent);
    ostr << "⎛" << std::endl;
    tab(indent+1);
    p(statemachine.stateset);
    BOOST_FOREACH(in_clause_doc in_clause, statemachine.in_clauses) {
      p(in_clause, indent+1);
    }
    ostr << "⎝" << std::endl;
  }

  void operator()(std::string const& s) const {
    ostr << s;
  }

  void operator()(placeholder_list_doc const& placeholder_list) const {
    hbc_node_printer p;
    tab(indent);
    ostr << "placeholders: ";
    BOOST_FOREACH(placeholder_doc placeholder, placeholder_list.placeholders) {
      p(placeholder);
      ostr << " ";
    }
  }

  void operator()(module_doc const& module) const {
    hbc_node_printer p;
    tab(indent);
    ostr << "[" << module.lineno << "] module <" << module.name << ">" << std::endl;
    tab(indent);
    ostr << "⎛" << std::endl;
    tab(indent + 1);
    p(module.placeholderlist);
    ostr << std::endl;
    tab(indent + 1);
    p(module.stateset);
    BOOST_FOREACH(in_clause_doc in_clause, module.in_clauses) {
      p(in_clause, indent+1);
    }
    tab(indent);
    ostr << "⎝" << std::endl;
  }

  void operator()(instantiation_doc const& instantiation) const {
    hbc_node_printer p;
    ostr << "Module instantiation in line " << instantiation.lineno
      << " Name: " << instantiation.name << " Module class: "
      << instantiation.moduleclass << std::endl;
    ostr << "Parameters" << std::endl;
    BOOST_FOREACH(inst_parameter_doc inst_parameter, instantiation.parameters) { // TODO this code "works"
      if(inst_parameter.which() == 0)
        boost::apply_visitor(hbc_node_printer(indent, ostr), boost::get<constant_doc>(inst_parameter));
      else if(inst_parameter.which() == 1)
        p(boost::get<float>(inst_parameter));
    }
  }

  void operator()(hbc_block const& block) const {
    hbc_node_printer p;
    ostr << "[Error: Block Type not defined]" << std::endl;
  }

  int indent;
  std::ostream& ostr;
};

void hbc_printer::operator()(hbc_doc const& hbc) const {
  BOOST_FOREACH(hbc_block const& block, hbc.blocks)
  {
    boost::apply_visitor(hbc_node_printer(indent), block);
    std::cout << std::endl;
  }
}

void hbc_printer::operator()(condition_doc const& cond, std::ostream& ostr) const {
  boost::apply_visitor(hbc_node_printer(0, ostr), cond);
}

void hbc_printer::operator()(command_block_doc const& command_block, std::ostream& ostr) const {
  hbc_node_printer p(0, ostr);
  p(command_block);
}
