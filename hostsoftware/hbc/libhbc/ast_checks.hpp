#ifndef LIBHBC_AST_CHECKS_HPP
#define LIBHBC_AST_CHECKS_HPP

#include <libhbc/common.hpp>
#include <libhbc/ast_datatypes.hpp>

namespace hexabus {

  class AstChecks {
    public:
      typedef std::tr1::shared_ptr<AstChecks> Ptr;
      AstChecks() {};
      virtual ~AstChecks() {};

      void operator()(hbc_doc& hbc);
  };
};

#endif // LIBHBC_AST_CHECKS_HPP
