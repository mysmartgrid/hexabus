#include "base64.hpp"

std::string hexabus::to_base64(std::vector<unsigned char> data) {
  // lookup table for the conversion
  unsigned char base64codes[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
  };

  // pad with 0x00 until length is multiple of 3
  unsigned short int pad = 0;
  while(data.size() % 3) {
    pad++;
    data.push_back(0x00);
  }

  std::vector<unsigned int> result;

  // we are sure that data.size is a multiple of 3, therefore we can increment by steps of 3
  for(unsigned int i = 0; i < data.size(); i += 3)
  {
    result.push_back(base64codes[ data[i] >> 2 ]);
    result.push_back(base64codes[ ((data[i] << 4) & 0x3f) | (data[i+1] >> 4) ]);
    result.push_back(base64codes[ ((data[i+1] << 2) & 0x3f)   | (data[i+2] >> 6) ]);
    result.push_back(base64codes[ (data[i+2] & 0x3f) ]);

    if(i == data.size() - 3) // we are in last triplet of bytes
    {
      if(pad == 2) // if two bytes were padded, last two sextets are turned into '='.
        result[result.size() - 2] = result[result.size() - 1] = '=';
      else if(pad == 1) // if one byte were padded, only the last sextet is '='.
        result.back() = '=';
    }
  }

  // convert vector to string -- TODO work with a string from the beginning.
  std::string res(result.begin(), result.end());
  return res;
}

