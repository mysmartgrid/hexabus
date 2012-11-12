#ifndef LIBHBA_HBA_PRINTER_HPP
#define LIBHBA_HBA_PRINTER_HPP 1

///////////////////////////////////////////////////////////////////////////
//  Print out the HexaBus Assembler
///////////////////////////////////////////////////////////////////////////

#include <libhba/common.hpp>
#include <libhba/ast_datatypes.hpp>
#include <iostream>


namespace hexabus {
  
  int const tabsize = 4;
  void tab(int indent);

  struct hba_printer
  {
	hba_printer(int indent = 0)
	  : indent(indent)
	{}

	void operator()(hba_doc const& hba) const;

	int indent;
  };


};


#endif /* LIBHBA_HBA_PRINTER_HPP */

