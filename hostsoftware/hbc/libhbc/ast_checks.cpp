#include "ast_checks.hpp"

#include <libhbc/error.hpp>

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
    // - later (when we have the placeholder-table and stuff), check placeholders, endpoint / device names, ...

    // check if init state is present in stateset
    if(!contains(statemachine.stateset.states, "init")) {
      std::ostringstream oss;
      oss << "[" << statemachine.stateset.lineno << "] Stateset does not contain init state." << std::endl;
      throw NoInitStateException(oss.str());
    }

    // check all the in clauses
    BOOST_FOREACH(in_clause_doc in_clause, statemachine.in_clauses) {
      if(!contains(statemachine.stateset.states, in_clause.name)) {
        std::ostringstream oss;
        oss << "[" << in_clause.lineno << "] In clause from nonexistent state \"" << in_clause.name << "\"." << std::endl;
        throw NonexistentStateException(oss.str());
      }

      // now check all the goto's in the in clause.
      BOOST_FOREACH(if_clause_doc if_clause, in_clause.if_clauses) {
        // the if block
        if(!contains(statemachine.stateset.states, if_clause.if_block.command_block.goto_command.target_state)) {
          std::ostringstream oss;
          oss << "[" << if_clause.if_block.command_block.goto_command.lineno << "] Goto target state \"" << if_clause.if_block.command_block.goto_command.target_state << "\" does not exist." << std::endl;
          throw NonexistentStateException(oss.str());
        }

        // the else-if blocks
        BOOST_FOREACH(guarded_command_block_doc com_block, if_clause.else_if_blocks) {
          if(!contains(statemachine.stateset.states, com_block.command_block.goto_command.target_state)) {
            std::ostringstream oss;
            oss << "[" << com_block.command_block.goto_command.lineno << "] Goto target state \"" << com_block.command_block.goto_command.target_state << "\" does not exist." << std::endl;
            throw NonexistentStateException(oss.str());
          }
        }

        // the else block
        if(if_clause.else_clause.present) {
          if(!contains(statemachine.stateset.states, if_clause.else_clause.commands.goto_command.target_state)) {
            std::ostringstream oss;
            oss << "[" << if_clause.else_clause.commands.goto_command.lineno << "] Goto target state \"" << if_clause.else_clause.commands.goto_command.target_state << "\" does not exist." << std::endl;
            throw NonexistentStateException(oss.str());
          }
        }
      }
    }
  }

  void operator()(module_doc module) const {
    // TODO also check all the placeholders OR decide to check them in the module instantiation code later!

    if(!contains(module.stateset.states, "init")) {
      std::ostringstream oss;
      oss << "[" << module.stateset.lineno << "] Stateset does not contain init state." << std::endl;
      throw NoInitStateException(oss.str());
    }

    // check all the in clauses
    BOOST_FOREACH(in_clause_doc in_clause, module.in_clauses) {
      if(!contains(module.stateset.states, in_clause.name)) {
        std::ostringstream oss;
        oss << "[" << in_clause.lineno << "] In clause from nonexistent state \"" << in_clause.name << "\"." << std::endl;
        throw NonexistentStateException(oss.str());
      }

      // now check all the goto's in the in clause.
      BOOST_FOREACH(if_clause_doc if_clause, in_clause.if_clauses) {
        // the if block
        if(!contains(module.stateset.states, if_clause.if_block.command_block.goto_command.target_state)) {
          std::ostringstream oss;
          oss << "[" << if_clause.if_block.command_block.goto_command.lineno << "] Goto target state \"" << if_clause.if_block.command_block.goto_command.target_state << "\" does not exist." << std::endl;
          throw NonexistentStateException(oss.str());
        }

        // the else-if blocks
        BOOST_FOREACH(guarded_command_block_doc com_block, if_clause.else_if_blocks) {
          if(!contains(module.stateset.states, com_block.command_block.goto_command.target_state)) {
            std::ostringstream oss;
            oss << "[" << com_block.command_block.goto_command.lineno << "] Goto target state \"" << com_block.command_block.goto_command.target_state << "\" does not exist." << std::endl;
            throw NonexistentStateException(oss.str());
          }
        }

        // the else block
        if(if_clause.else_clause.present) {
          if(!contains(module.stateset.states, if_clause.else_clause.commands.goto_command.target_state)) {
            std::ostringstream oss;
            oss << "[" << if_clause.else_clause.commands.goto_command.lineno << "] Goto target state \"" << if_clause.else_clause.commands.goto_command.target_state << "\" does not exist." << std::endl;
            throw NonexistentStateException(oss.str());
          }
        }
      }
    }
  }

  void operator()(include_doc& include) const { }
  void operator()(endpoint_doc& ep) const { } // TODO here: check if there are multiple datatype definitions. BUT: We can also check this when we build the endpoint table, so we do it there. even more so: Check if we have multiple of anything.
  void operator()(alias_doc alias) const { } // TODO here: check if we have any command more then once. BUT: we can do this when we build the device list table. so we're gonna do it there.
};

void AstChecks::operator()(hbc_doc& hbc) {
  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    boost::apply_visitor(ast_checker(), block);
  }
}
