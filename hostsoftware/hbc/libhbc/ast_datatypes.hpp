#ifndef LIBHBC_AST_DATATYPES
#define LIBHBC_AST_DATATYPES

namespace hexabus {

  struct include_doc {
    std::string filename;
  };

  struct alias_doc {
    unsigned int lineno;
    std::string ipv6_address;
    std::vector<unsigned int> eids;
  };

  struct statemachine_doc {
    unsigned int lineno;
    std::vector<std::string> states;
    std::vector<in_clause_doc> in_clauses;
  };

  struct in_clause_doc {
    unsigned int lineno;
    std::vector<if_clause_doc> if_clauses;
  };

  struct if_clause_doc {
    unsigned int lineno;
    condition_doc condition;
    std::vector<command_doc> commands;
    goto_command_doc goto_command;
  };

  struct condition_doc {
    // TODO
  };

  struct command_doc {
    unsigned int lineno;
    write_command_doc write_command; // TODO boost::variant if there's more
  };

  struct goto_command_doc {
    unsigned int lineno;
    std::string target_state;
  };
};

#endif // LIBHBC_AST_DATATYPES

