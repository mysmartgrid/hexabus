#ifndef LIBHEXANODE_CALLBACKS_PRINT_CALLBACK_HPP
#define LIBHEXANODE_CALLBACKS_PRINT_CALLBACK_HPP 1

#include <libhexanode/common.hpp>
#include <libhexanode/callbacks/callback.hpp>
#include <vector>

namespace hexanode {
  class PrintCallback : public Callback {
    public:
      typedef boost::shared_ptr<PrintCallback> Ptr;
      PrintCallback () : Callback() {};
      virtual ~PrintCallback() {};

      void on_event(
        std::vector< unsigned char > *message);

    private:
      PrintCallback (const PrintCallback& original);
      PrintCallback& operator= (const PrintCallback& rhs);

  };
};

#endif /* LIBHEXANODE_CALLBACKS_PRINT_CALLBACK_HPP */
