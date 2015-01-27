#ifndef LIBHEXANODE_KEY_PRESSED_EVENT_HPP
#define LIBHEXANODE_KEY_PRESSED_EVENT_HPP 1

#include <libhexanode/common.hpp>
#include <libhexanode/event.hpp>

namespace hexanode {
  class KeyPressedEvent : public Event{
    public:
      KeyPressedEvent (event_payload_t payload) : Event(payload) {};
      virtual ~KeyPressedEvent() {};
      virtual std::string str();

    private:
      KeyPressedEvent (const KeyPressedEvent& original);
      KeyPressedEvent& operator= (const KeyPressedEvent& rhs);
      
  };
};


#endif /* LIBHEXANODE_KEY_PRESSED_EVENT_HPP */

