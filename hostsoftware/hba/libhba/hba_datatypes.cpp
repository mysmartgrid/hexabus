#include <libhba/hba_datatypes.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace hexabus;

Datatypes::Datatypes(std::string filename)
{
  // clear array
  for(unsigned int i = 0; i < NUMBER_OF_EIDS; i++)
    datatypes[i] = 0;
  // read file
  std::string line;
  std::ifstream f(filename.c_str());
  if(f.is_open())
  {
    while(f.good())
    {
      getline(f, line);
      std::stringstream s(line);

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
    std::cout << "Error opening datatype definition file." << std::endl;
  }
}

uint8_t Datatypes::getDatatype(uint8_t eid)
{
  if(eid < NUMBER_OF_EIDS)
    return datatypes[eid];
  else
    return 0;
}

