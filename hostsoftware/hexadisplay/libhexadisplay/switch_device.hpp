#ifndef LIBHEXADISPLAY_SWITCH_DEVICE_HPP
#define LIBHEXADISPLAY_SWITCH_DEVICE_HPP 1

#include <libhexabus/socket.hpp>
#include <libhexabus/packet.hpp>

namespace hexadisplay {
  class SwitchDevice {
    public:
      typedef std::tr1::shared_ptr<SwitchDevice> Ptr;
      SwitchDevice(
          const std::string& interface, 
          const std::string& ip
        );
      SwitchDevice(
          const std::string& configfile
        );
      virtual ~SwitchDevice();
      void on();
      void off();
      void toggle();
      bool is_on() {return _switch_state;};

    private:
      SwitchDevice (const SwitchDevice& original);
      SwitchDevice& operator= (const SwitchDevice& rhs);
      boost::asio::io_service _io;
      hexabus::Socket _socket;
      std::string _ip;
      bool _switch_state;

      static std::string configIP(const std::string& configfile);
      static std::string configInterface(const std::string& configfile);
  };
};


#endif /* LIBHEXADISPLAY_SWITCH_DEVICE_HPP */

