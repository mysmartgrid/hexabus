#include "ast_checks.hpp"

using namespace hexabus;

struct ast_checker : boost::static_visitor<> {
  ast_checker() { }

  void operator()(statemachine_doc& statemachine) const {
    // TODO
    // - if possible: check if all goto-states are in the stateset / there is no invalid goto
    // - later (when we have the placeholder-table and stuff), check placeholders, endpoint / device names, ...

    // check if init state is present in stateset
    bool init_state_present;
    BOOST_FOREACH(std::string state_name, statemachine.stateset.states) {
      if("init" == state_name)
        init_state_present = true;
    }
    if(!init_state_present) {
      // TODO throw something
    }

    std::cout << "Machine: " << statemachine.name << " -- Init state present: " << init_state_present << std::endl;
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
