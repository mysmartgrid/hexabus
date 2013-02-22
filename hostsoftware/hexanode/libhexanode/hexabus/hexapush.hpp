#ifndef LIBHEXANODE_HEXABUS_HEXAPUSH_HPP
#define LIBHEXANODE_HEXABUS_HEXAPUSH_HPP 1

#include <libhexanode/common.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/packet.hpp>

namespace hexanode {
  /***
   * This class simulates a device that implements
   * EID 25 aka HexaPush.
   */
  class HexaPush {
    public:
      typedef boost::shared_ptr<HexaPush> Ptr;
      HexaPush (hexabus::Socket& socket);
      virtual ~HexaPush() {};

      // incoming events
      void on_event(uint8_t pressed_key);


    private:
      HexaPush (const HexaPush& original);
      HexaPush& operator= (const HexaPush& rhs);
      hexabus::Socket& _socket;
  };
};


#endif /* LIBHEXANODE_HEXABUS_HEXAPUSH_HPP */

