#ifndef LIBHEXANODE_HISTORIAN_HPP
#define LIBHEXANODE_HISTORIAN_HPP 1

#include <libhexanode/common.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include <map>

namespace hexanode {
  typedef std::map<boost::asio::ip::address, uint32_t> devices_t;
  class Historian {
    public:
      typedef boost::shared_ptr<Historian> Ptr;
      Historian(uint32_t update_seconds) : _power_consumptions()
        , _power_production()
        , _t()
        , _terminate_midievent_loop(false)
        , _update_seconds(update_seconds)
        {};
      virtual ~Historian() {}; 
      void send_power_balance();
      void set_production(uint32_t current_production);
      void add_consumption(
          const boost::asio::ip::udp::endpoint& endpoint,
          const uint32_t consumption);
      void run();
      void shutdown();

    private:
      Historian (const Historian& original);
      Historian& operator= (const Historian& rhs);
      void loop();

      devices_t _power_consumptions;
      boost::mutex _guard;
      uint32_t _power_production;
      boost::thread _t;
      bool _terminate_midievent_loop;
      uint32_t _update_seconds;

  };

};


#endif /* LIBHEXANODE_HISTORIAN_HPP */

