#ifndef LIBHEXANODE_HEXABUS_HEXADIAL_HPP
#define LIBHEXANODE_HEXABUS_HEXADIAL_HPP 1

#include <libhexanode/common.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/packet.hpp>
#include <map>
#include <boost/thread/mutex.hpp>

namespace hexanode {
  /***
   * This class simulates a device that implements
   * EIDs 33-40 - it uses generic dial gauges to represent uint8 values.
   */
  class HexaDial {
    public:
      typedef boost::shared_ptr<HexaDial> Ptr;
      typedef std::map<uint8_t,uint8_t> dial_positions_t;
      typedef std::map<uint8_t,uint8_t>::iterator dial_positions_it_t;
      HexaDial (hexabus::Socket& socket)
        : _socket(socket)
        , _values()
        , _old_values() 
        , _mtx() {};
      virtual ~HexaDial() {};

      // incoming events
      void on_event(const uint8_t dial_turned, const uint8_t dial_value);
      // trigger broadcast of last value
      void send_values();

    private:
      HexaDial (const HexaDial& original);
      HexaDial& operator= (const HexaDial& rhs);
      template <typename Map>
        bool map_equals (Map const &lhs, Map const &rhs);
      hexabus::Socket& _socket;
      dial_positions_t _values;
      dial_positions_t _old_values;
      boost::mutex _mtx;
  };

  template <typename Map>
    bool map_equal (Map const &lhs, Map const &rhs) {
      // No predicate needed because there is operator== for pairs already.
      return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(),
            rhs.begin());
    }
};


#endif /* LIBHEXANODE_HEXABUS_HEXADIAL_HPP */

