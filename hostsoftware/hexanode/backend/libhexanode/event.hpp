#ifndef LIBHEXANODE_EVENT_HPP
#define LIBHEXANODE_EVENT_HPP 1

#include <libhexanode/common.hpp>

namespace hexanode {
  typedef uint16_t event_payload_t;
  class Event {
    public:
      Event (event_payload_t payload) : _payload(payload) {};
      virtual ~Event() {};
      event_payload_t payload() { return _payload; };
      virtual std::string str() = 0;

    protected:
      event_payload_t _payload;

    private:
      Event (const Event& original);
      Event& operator= (const Event& rhs);
      
  };
};

#endif /* LIBHEXANODE_EVENT_HPP */
