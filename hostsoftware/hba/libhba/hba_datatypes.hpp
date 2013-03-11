#ifndef HBA_DATATYPES_HPP
#define HBA_DATATYPES_HPP

#include <stdint.h>
#include <string>
#include "../../../shared/hexabus_types.h"

#define NUMBER_OF_EIDS 32

namespace hexabus {
  class Datatypes {
    public:
      static Datatypes* getInstance(std::string filename = "datatypes");
      hxb_datatype getDatatype(uint8_t eid);
    private:
      static Datatypes* instance;
      hxb_datatype datatypes[NUMBER_OF_EIDS];
      Datatypes(std::string filename);
      Datatypes();
  };
};

#endif
