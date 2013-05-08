#include "dtype_file_generator.hpp"

#include <libhbc/error.hpp>

using namespace hexabus;

void DTypeFileGenerator::operator()(endpoint_table_ptr ept, std::ofstream &ofs) {
  for(endpoint_table::iterator it = ept->begin(); it != ept->end(); ++it) {
    ofs << it->second.eid;
    ofs << " ";
    switch(it->second.dtype) {
      case DT_UNDEFINED:
        ofs << "UNDEFINED";
        break;
      case DT_BOOL:
        ofs << "BOOL";
        break;
      case DT_UINT8:
        ofs << "UINT8";
        break;
      case DT_UINT32:
        ofs << "UINT32";
        break;
      case DT_FLOAT:
        ofs << "FLOAT";
        break;
      default:
        {
          std::ostringstream oss;
          oss << "Unsupported data type for endpoint ID " << it->second.eid << "." << std::endl;
          throw UnsupportedDatatypeException(oss.str());
        }
        break;
    }

    ofs << std::endl;
  }
}

