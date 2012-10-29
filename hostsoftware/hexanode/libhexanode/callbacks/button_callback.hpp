#ifndef LIBHEXANODE_CALLBACKS_BUTTON_CALLBACK_HPP
#define LIBHEXANODE_CALLBACKS_BUTTON_CALLBACK_HPP 1

#include <libhexanode/common.hpp>
#include <libhexanode/callbacks/callback.hpp>
#include <vector>

namespace hexanode {
  class ButtonCallback : public Callback {
    public:
      typedef boost::shared_ptr<ButtonCallback> Ptr;
      ButtonCallback () : Callback() {};
      virtual ~ButtonCallback() {};

      void on_event(
        std::vector< unsigned char > *message);

    private:
      ButtonCallback (const ButtonCallback& original);
      ButtonCallback& operator= (const ButtonCallback& rhs);

  };
};

#endif /* LIBHEXANODE_CALLBACKS_BUTTON_CALLBACK_HPP */
