#ifndef LIBHEXANODE_KEY_PRESSED_EVENT_HPP
#define LIBHEXANODE_KEY_PRESSED_EVENT_HPP 1

#include <libhexanode/common.hpp>
#include <libhexanode/event.hpp>

namespace hexanode {
  class KeyPressedEvent : public Event{
    public:
      typedef boost::shared_ptr<KeyPressedEvent> Ptr;
      KeyPressedEvent (uint16_t id) : Event(id) {};
      virtual ~KeyPressedEvent() {};
      virtual std::string str();

    private:
      KeyPressedEvent (const KeyPressedEvent& original);
      KeyPressedEvent& operator= (const KeyPressedEvent& rhs);
      
  };
};


#endif /* LIBHEXANODE_KEY_PRESSED_EVENT_HPP */

