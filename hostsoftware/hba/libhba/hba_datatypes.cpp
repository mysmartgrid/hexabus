#include <libhba/hba_datatypes.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <libhba/common.hpp>

using namespace hexabus;

Datatypes* Datatypes::instance = NULL; // Singleton
Datatypes* Datatypes::getInstance(std::string filename)
{
  if(instance == NULL)
    instance = new Datatypes(filename);

  return instance;

  // Note: This only works under the assumption that the "filename" stays the same throughout one hbasm-run.
  // If we want to have multiple datatype definition files (we don't need that at the moment), we have to
  // keep track of multiple instances (vector<Datatypes*> instances)!
}

Datatypes::Datatypes(std::string filename)
{
  if("" != filename) {
    // read file
    std::string line;
    std::ifstream f(filename.c_str());
    if(f.is_open())
    {
      while(f.good())
      {
        getline(f, line);
        std::stringstream s(line);

        if(line.length() == 0)
          continue;

        int eid;
        std::string dtype;
        s >> eid;
        s >> dtype;

        if(!dtype.compare("UNDEFINED"))
          datatypes[eid] = HXB_DTYPE_UNDEFINED;
        else if(!dtype.compare("BOOL"))
          datatypes[eid] = HXB_DTYPE_BOOL;
        else if(!dtype.compare("UINT8"))
          datatypes[eid] = HXB_DTYPE_UINT8;
        else if(!dtype.compare("UINT32"))
          datatypes[eid] = HXB_DTYPE_UINT32;
        else if(!dtype.compare("DATETIME"))
          datatypes[eid] = HXB_DTYPE_DATETIME;
        else if(!dtype.compare("FLOAT"))
          datatypes[eid] = HXB_DTYPE_FLOAT;
        else if(!dtype.compare("128STRING"))
          datatypes[eid] = HXB_DTYPE_128STRING;
        else if(!dtype.compare("TIMESTAMP"))
          datatypes[eid] = HXB_DTYPE_TIMESTAMP;
        else
          datatypes[eid] = HXB_DTYPE_UNDEFINED;
      }

      f.close();
    } else {
      std::ostringstream oss;
      oss << "Datatype definition file " << filename << " could not be opened.";
      throw DatatypeNotFoundException(oss.str());
    }
  }
}

hxb_datatype Datatypes::getDatatype(uint32_t eid)
{
  std::map<uint32_t, hxb_datatype>::iterator datatype = datatypes.find(eid);
  if(datatype != datatypes.end())
  {
    return datatype->second;
  } else {
    std::ostringstream oss;
    oss << "No datatype definition found for endpoint ID " << eid << ".";
    throw DatatypeNotFoundException(oss.str());
  }
}

