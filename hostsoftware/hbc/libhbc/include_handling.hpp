#ifndef LIBHBC_INCLUDE_HANDLING
#define LIBHBC_INCLUDE_HANDLING

#include <libhbc/ast_datatypes.hpp>
#include <libhbc/error.hpp>

#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace hexabus {
  class IncludeHandling {
    public:
      IncludeHandling(std::string main_file_name);

      fs::path operator[](unsigned int index);
      unsigned int size();
      void addFileName(include_doc incl);

    private:
      std::vector<fs::path> filenames; // store all the names of the files to be read
      fs::path base_dir; // the base path ouf our main file; everything up to the directory the file is in, not just up to the working directory. used for finding includes
  };
};

#endif // LIBHBC_INCLUDE_HANDLING
