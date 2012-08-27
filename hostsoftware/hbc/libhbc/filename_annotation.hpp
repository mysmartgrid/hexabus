#ifndef LIBHBC_FILENAME_ANNOTATION_HPP
#define LIBHBC_FILENAME_ANNOTATION_HPP

#include "libhbc/ast_datatypes.hpp"

namespace hexabus {

  class FilenameAnnotation {
    public:
      typedef std::tr1::shared_ptr<FilenameAnnotation> Ptr;
      FilenameAnnotation(std::string fn) { _f = fn; };
      virtual ~FilenameAnnotation() {};

      void operator()(hbc_doc& hbc);

    private:
      std::string _f;
  };
};

#endif // LIBHBC_FILENAME_ANNOTATION_HPP
