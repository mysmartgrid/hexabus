#include "ast_checks.hpp"

using namespace hexabus;

bool hexabus::contains(std::vector<std::string> v, std::string s) {
  BOOST_FOREACH(std::string e, v) {
    if(s == e)
      return true;
  }
  return false;
}

struct ast_checker : boost::static_visitor<> {
  ast_checker() { }

  void operator()(statemachine_doc& statemachine) const {
    // TODO
    // - if possible: check if all goto-states are in the stateset / there is no invalid goto
    // - later (when we have the placeholder-table and stuff), check placeholders, endpoint / device names, ...

    // check if init state is present in stateset
    if(!contains(statemachine.stateset.states, "init")) {
      // TODO throw something
      std::cout << "Stateset does not contain init state in line " << statemachine.stateset.lineno << std::endl;
    }

    // check all the in clauses
    BOOST_FOREACH(in_clause_doc in_clause, statemachine.in_clauses) {
      if(!contains(statemachine.stateset.states, in_clause.name)) {
        // TODO throw something
        std::cout << "In clause from nonexistent state \"" << in_clause.name << "\" in line " << in_clause.lineno << std::endl;
      }

      // now check all the goto's in the in clause.
      BOOST_FOREACH(if_clause_doc if_clause, in_clause.if_clauses) {
        // the if block
        if(!contains(statemachine.stateset.states, if_clause.if_block.command_block.goto_command.target_state)) {
          // TODO throw something
          std::cout << "Goto target state \"" << if_clause.if_block.command_block.goto_command.target_state << "\" does not exist in line " << if_clause.if_block.command_block.goto_command.lineno << std::endl;
        }

        // the else-if blocks
        BOOST_FOREACH(guarded_command_block_doc com_block, if_clause.else_if_blocks) {
          if(!contains(statemachine.stateset.states, com_block.command_block.goto_command.target_state)) {
            // TODO throw something
            std::cout << "Goto target state \"" << com_block.command_block.goto_command.target_state << "\" does not exist in line " << com_block.command_block.goto_command.lineno << std::endl;
          }
        }

        // the else block
        if(if_clause.else_clause.present) {
          if(!contains(statemachine.stateset.states, if_clause.else_clause.commands.goto_command.target_state)) {
            // TODO throw something
            std::cout << "Goto target state \"" << if_clause.else_clause.commands.goto_command.target_state << "\" does not exist in line " << if_clause.else_clause.commands.goto_command.lineno << std::endl;
          }
        }
      }
    }
  }

  void operator()(include_doc& include) const { }
  void operator()(endpoint_doc& ep) const { } // TODO here: check if there are multiple datatype definitions. BUT: We can also check this when we build the endpoint table, so we do it there. even more so: Check if we have multiple of anything.
  void operator()(alias_doc alias) const { } // TODO here: check if we have any command more then once. BUT: we can do this when we build the device list table. so we're gonna do it there.
  void operator()(module_doc module) const { } // TODO here: Same checks as for statemachine, PLUS check if all placeholders are declared correctly. BUT we could also check placeholders on module instantiation (???)
};

void AstChecks::operator()(hbc_doc& hbc) {
  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    boost::apply_visitor(ast_checker(), block);
  }
}
