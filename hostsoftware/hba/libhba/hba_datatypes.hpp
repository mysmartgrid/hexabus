#ifndef HBA_DATATYPES_HPP
#define HBA_DATATYPES_HPP

#include <stdint.h>
#include <string>
#include <map>
#include "../../../shared/hexabus_types.h"

namespace hexabus {
  class Datatypes {
    public:
      static Datatypes* getInstance(std::string filename = "datatypes");
      hxb_datatype getDatatype(uint32_t eid);
    private:
      static Datatypes* instance;
      std::map<uint32_t, hxb_datatype> datatypes;
      Datatypes(std::string filename);
      Datatypes();
  };
};

#endif
