#ifndef LIBHBA_BASE64_HPP
#define LIBHBA_BASE64_HPP

#include <libhba/common.hpp>
#include <stdint.h>
#include <vector>

namespace hexabus {
  std::string to_base64(std::vector<unsigned char> data);
};

#endif /* LIBHBA_BASE64_HPP */

