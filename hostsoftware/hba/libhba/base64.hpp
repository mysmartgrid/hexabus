#ifndef LIBHBA_BASE64_HPP
#define LIBHBA_BASE64_HPP 1

#include <libhba/common.hpp>
#include <stdint.h>
#include <vector>

// See http://www.codeproject.com/Articles/359160/Base64-Encoder-and-Boost

namespace hexabus {
  std::string to_base64(std::vector<unsigned char> data);
  std::vector<unsigned char> from_base64(std::string data);
};


#endif /* LIBHBA_BASE64_HPP */

