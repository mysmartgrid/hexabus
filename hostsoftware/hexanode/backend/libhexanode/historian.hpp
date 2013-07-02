#ifndef LIBHEXANODE_HISTORIAN_HPP
#define LIBHEXANODE_HISTORIAN_HPP 1

#include <libhexanode/common.hpp>
#include <libhexabus/socket.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include <map>

namespace hexanode {
  typedef std::map<std::string, uint32_t> devices_t;
  class Historian {
    public:
      typedef boost::shared_ptr<Historian> Ptr;
      Historian(
          hexabus::Socket* send_socket,
          uint32_t update_seconds) 
        : _power_consumptions()
        , _power_productions()
        , _t()
        , _terminate_loop(false)
        , _update_seconds(update_seconds)
        , _send_socket(send_socket)
        {};
      virtual ~Historian() {}; 
      void send_power_balance();
      void add_production(
          const boost::asio::ip::udp::endpoint& endpoint,
          const uint32_t& eid,
          uint32_t current_production);
      void add_consumption(
          const boost::asio::ip::udp::endpoint& endpoint,
          const uint32_t& eid,
          const uint32_t consumption);
      void remove_device(
          const boost::asio::ip::udp::endpoint& endpoint,
          const uint32_t& eid);
      void run();
      void shutdown();

    private:
      Historian (const Historian& original);
      Historian& operator= (const Historian& rhs);
      void loop();
      const std::string endpoint_eid2device(
          const boost::asio::ip::udp::endpoint& endpoint,
          const uint32_t& eid);

      boost::mutex _guard;
      devices_t _power_consumptions;
      devices_t _power_productions;
      boost::thread _t;
      bool _terminate_loop;
      uint32_t _update_seconds;
      hexabus::Socket* _send_socket;

  };

};


#endif /* LIBHEXANODE_HISTORIAN_HPP */

