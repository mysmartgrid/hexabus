#ifndef LIBHEXANODE_CALLBACKS_CALLBACK_HPP
#define LIBHEXANODE_CALLBACKS_CALLBACK_HPP 1

#include <libhexanode/common.hpp>
#include <vector>

namespace hexanode {
  class Callback {
    public:
      typedef boost::shared_ptr<Callback> Ptr;
      Callback() {};
      virtual ~Callback() {};

      virtual void on_event(
        std::vector< unsigned char > *message) = 0;

    private:
      Callback (const Callback& original);
      Callback& operator= (const Callback& rhs);
      
  };
};


#endif /* LIBHEXANODE_CALLBACKS_CALLBACK_HPP */

