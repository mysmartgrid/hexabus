#ifndef LIBHEXANODE_CALLBACKS_BUTTON_CALLBACK_HPP
#define LIBHEXANODE_CALLBACKS_BUTTON_CALLBACK_HPP 1

#include <libhexanode/common.hpp>
#include <boost/signals2.hpp>
#include <libhexanode/callbacks/callback.hpp>
#include <vector>

namespace hexanode {
  class ButtonCallback : public Callback {
    public:
      typedef boost::shared_ptr<ButtonCallback> Ptr;
      typedef boost::signals2::signal<void 
        (uint8_t)> on_buttonevent_t;
      typedef boost::signals2::signal<void 
        (uint8_t, uint8_t)> on_dialevent_t;
      typedef on_buttonevent_t::slot_type on_buttonevent_slot_t;
      typedef on_dialevent_t::slot_type on_dialevent_slot_t;
      ButtonCallback (); 
      virtual ~ButtonCallback() {};

      // incomming events
      void on_event(
        std::vector< unsigned char > *message);
      
      // register callbacks to be called on events.
      boost::signals2::connection do_on_buttonevent(
          const on_buttonevent_slot_t& slot);
      boost::signals2::connection do_on_dialevent(
          const on_dialevent_slot_t& slot);


    private:
      ButtonCallback (const ButtonCallback& original);
      ButtonCallback& operator= (const ButtonCallback& rhs);
      on_buttonevent_t _on_buttonevent;
      on_dialevent_t _on_dialevent;

  };
};

#endif /* LIBHEXANODE_CALLBACKS_BUTTON_CALLBACK_HPP */
