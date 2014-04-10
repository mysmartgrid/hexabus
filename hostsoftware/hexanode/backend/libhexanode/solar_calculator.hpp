#ifndef LIBHEXANODE_SOLAR_CALCULATOR_HPP
#define LIBHEXANODE_SOLAR_CALCULATOR_HPP 1

#include <libhexanode/common.hpp>
#include <libhexanode/error.hpp>
#include <libhexanode/historian.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>

namespace hexanode {
  class SolarCalculator : public hexabus::PacketVisitor {
    public:
      typedef boost::shared_ptr<SolarCalculator> Ptr;
      SolarCalculator(
          hexabus::Listener* receive_socket,
          hexabus::Socket* send_socket,
          const boost::asio::ip::udp::endpoint& endpoint,
          const uint32_t& pv_production_eid,
          const uint32_t& pv_peak_watt,
          const uint32_t& battery_eid,
          const uint32_t& battery_peak_watt,
          Historian::Ptr historian
          )
        : _receive_socket(receive_socket)
        , _send_socket(send_socket)
        , _endpoint(endpoint)
        , _pv_production_eid(pv_production_eid)
        , _pv_peak_watt(pv_peak_watt)
        , _battery_eid(battery_eid)
        , _battery_peak_watt(battery_peak_watt)
        , _historian(historian)
       {};
      virtual ~SolarCalculator() {};


    private:
      SolarCalculator (const SolarCalculator& original);
      SolarCalculator& operator= (const SolarCalculator& rhs);
      hexabus::Listener* _receive_socket;
      hexabus::Socket* _send_socket;
      boost::asio::ip::udp::endpoint _endpoint;
      const uint32_t _pv_production_eid;
      uint32_t _pv_peak_watt;
      const uint32_t _battery_eid;
      uint32_t _battery_peak_watt;
      hexanode::Historian::Ptr _historian;

      void push_value(const uint32_t eid, const uint8_t value);
      void push_value(const uint32_t eid, const uint32_t value);
      void printValueHeader(uint32_t eid, const char* datatypeStr);

      template<typename T> 
        std::string numeric2string(T value) {
          std::ostringstream oss;
          oss << std::fixed << std::setprecision(1) << value;
          return oss.str();
        }


      template<typename T>
        void printValuePacket(const hexabus::ValuePacket<T>& packet, 
            const char* datatypeStr)
        {
          printValueHeader(packet.eid(), datatypeStr);
          std::cout << " Value:\t" << packet.value() << std::endl;
        }

      void printValuePacket(const hexabus::ValuePacket<uint8_t>& packet, 
          const char* datatypeStr);

      void printValuePacket (const hexabus::ValuePacket<uint32_t>& packet, 
          const char* datatypeStr);

      void printValuePacket (const hexabus::InfoPacket<float>& packet, 
          const char* datatypeStr)
      {
        std::cout << "Ignoring info packet." << std::endl;
      }


      template<size_t L>
        void printValuePacket(const hexabus::ValuePacket<boost::array<char, L> >& packet, const char* datatypeStr)
        {
          printValueHeader(packet.eid(), datatypeStr);

          std::stringstream hexstream;

          hexstream << std::hex << std::setfill('0');

          for (size_t i = 0; i < L; ++i)
          {
            hexstream << std::setw(2) << (0xFF & packet.value()[i]) << " ";
          }

          std::cout << std::endl << std::endl;

          std::cout << "Value:\t" << hexstream.str() << std::endl; 
          std::cout << std::endl;
        }


      /**
       * Other definitions
       */
    public:
      /**
       * defined in cpp
       */
      virtual void visit(const hexabus::ErrorPacket& error);
      virtual void visit(const hexabus::QueryPacket& query);
      virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery);
      virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo);
      virtual void visit(const hexabus::InfoPacket<bool>& info) { printValuePacket(info, "Bool"); }
      virtual void visit(const hexabus::InfoPacket<uint8_t>& info) { printValuePacket(info, "UInt8"); }
      virtual void visit(const hexabus::InfoPacket<uint32_t>& info) { printValuePacket(info, "UInt32"); }
      virtual void visit(const hexabus::InfoPacket<float>& info) { printValuePacket(info, "Float"); }
      virtual void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info) { printValuePacket(info, "Datetime"); }
      virtual void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info) { printValuePacket(info, "Timestamp"); }
      virtual void visit(const hexabus::InfoPacket<std::string>& info) { printValuePacket(info, "String"); }
      virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (16 bytes)"); }
      virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (65 bytes)"); }
      /** 
       * not needed.
       */
      virtual void visit(const hexabus::WritePacket<bool>& write) {}
      virtual void visit(const hexabus::WritePacket<uint8_t>& write) {}
      virtual void visit(const hexabus::WritePacket<uint32_t>& write) {}
      virtual void visit(const hexabus::WritePacket<float>& write) {}
      virtual void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) {}
      virtual void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) {}
      virtual void visit(const hexabus::WritePacket<std::string>& write) {}
      virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> >& write) {}
      virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >& write) {}

  };
};


#endif /* LIBHEXANODE_SOLAR_CALCULATOR_HPP */

