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
    }
  }

  void operator()(include_doc& include) const { }
  void operator()(endpoint_doc& ep) const { }
  void operator()(alias_doc alias) const { }
  void operator()(module_doc module) const { }
};

void AstChecks::operator()(hbc_doc& hbc) {
  BOOST_FOREACH(hbc_block block, hbc.blocks) {
    boost::apply_visitor(ast_checker(), block);
  }
}
