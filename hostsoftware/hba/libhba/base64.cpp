#include "base64.hpp"
#include <boost/serialization/pfto.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>
#include <boost/archive/iterators/transform_width.hpp>

// See http://www.webbiscuit.co.uk/2012/04/02/base64-encoder-and-boost/

std::vector<unsigned char> hexabus::from_base64(std::string input) {
  using namespace boost::archive::iterators;

  // Pad with 0 until a multiple of 3
  //uint8_t paddedCharacters = 0;
  //while(data.size() % 3 != 0) {
  //  paddedCharacters++;
  //  data.push_back(0x00);
  //}

  // Crazy typedef black magic
  typedef
    transform_width<   // retrieve 8 bit integers from a sequence of 6 bit bytes
      binary_from_base64<    // convert base64 characters to binary values 
        //boost::archive::iterators::remove_whitespace<
        const unsigned char *
        //>
        >
      ,8
      ,6
    >
    base64Iterator; // compose all the above operations in to a new iterator

  // Encode the buffer and create a string


  std::string decoded;
  decoded.append(base64Iterator(BOOST_MAKE_PFTO_WRAPPER(input.begin(), input.end()));

  // Add '=' for each padded character used
  //for(uint8_t i = 0; i < paddedCharacters; i++) {
  //  encoded_string.push_back('=');
  //}

  std::vector<unsigned char> retval;
  retval.resize(input.size());
  std::copy(decoded.begin(), retval.size(), retval.begin());
  return retval;
}

std::string hexabus::to_base64(std::vector<unsigned char> data) {
  using namespace boost::archive::iterators;

  // Pad with 0 until a multiple of 3
  uint8_t paddedCharacters = 0;
  while(data.size() % 3 != 0) {
    paddedCharacters++;
    data.push_back(0x00);
  }

  // Crazy typedef black magic
  typedef
    insert_linebreaks<         // insert line breaks every 76 characters
    base64_from_binary<    // convert binary values to base64 characters
    transform_width<   // retrieve 6 bit integers from a sequence of 8 bit bytes
    const uint8_t *
    ,6
    ,8
    >
    >
    ,76
    >
    base64Iterator; // compose all the above operations in to a new iterator

  // Encode the buffer and create a string
  std::string encoded_string(
      base64Iterator(&data[0]),
      base64Iterator(&data[0] + (data.size() - paddedCharacters)));

  // Add '=' for each padded character used
  for(uint8_t i = 0; i < paddedCharacters; i++) {
    encoded_string.push_back('=');
  }

  return encoded_string;
}

