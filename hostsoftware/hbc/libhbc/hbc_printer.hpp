#ifndef LIBHBC_HBC_PRINTER_HPP
#define LIBHBC_HBC_PRINTER_HPP

#include <libhbc/common.hpp>
#include <libhbc/ast_datatypes.hpp>
#include <iostream>

namespace hexabus {
  int const tabsize = 4;
  void tab(int indent);

  struct hbc_printer
  {
    hbc_printer(int indent = 0)
      : indent(indent)
    {}

    void operator()(hbc_doc const& hbc) const;
    void operator()(condition_doc const& cond, std::ostream& ostr) const;

    int indent;
  };
};

#endif /* LIBHBC_HBC_PRINTER_HPP */

