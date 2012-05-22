#ifndef LIBHEXADISPLAY_SWITCH_DEVICE_HPP
#define LIBHEXADISPLAY_SWITCH_DEVICE_HPP 1

#include <libhexabus/network.hpp>
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

    private:
      SwitchDevice (const SwitchDevice& original);
      SwitchDevice& operator= (const SwitchDevice& rhs);
      hexabus::NetworkAccess* _network;
      hexabus::Packet::Ptr _packetm;
      std::string _ip;

  };
};


#endif /* LIBHEXADISPLAY_SWITCH_DEVICE_HPP */

