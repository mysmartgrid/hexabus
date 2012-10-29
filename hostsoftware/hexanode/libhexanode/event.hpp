#ifndef LIBHEXANODE_EVENT_HPP
#define LIBHEXANODE_EVENT_HPP 1

#include <libhexanode/common.hpp>

namespace hexanode {
  class Event {
    public:
      typedef boost::shared_ptr<Event> Ptr;
      Event (uint16_t id) : _id(id) {};
      virtual ~Event() {};
      uint16_t id() { return _id; };
      virtual std::string str() = 0;

    protected:
      uint16_t _id;

    private:
      Event (const Event& original);
      Event& operator= (const Event& rhs);
      
  };
};


#endif /* LIBHEXANODE_EVENT_HPP */

