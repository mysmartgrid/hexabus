#ifndef LIBHEXANODE_PACKET_PUSHER_HPP
#define LIBHEXANODE_PACKET_PUSHER_HPP 1

#include <libhexanode/common.hpp>
#include <libhexabus/packet.hpp>

namespace hexanode {

  class PacketPusher : public hexabus::PacketVisitor {
    private:
      std::ostream& target;

      void printValueHeader(uint32_t eid, const char* datatypeStr);

      template<typename T>
        void printValuePacket(const hexabus::ValuePacket<T>& packet, const char* datatypeStr)
        {
          printValueHeader(packet.eid(), datatypeStr);
          target << "Value:\t" << packet.value() << std::endl;
          target << std::endl;
        }

      void printValuePacket(const hexabus::ValuePacket<uint8_t>& packet, const char* datatypeStr);

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

          target << "Value:\t" << hexstream.str() << std::endl; 
          target << std::endl;
        }


    public:
      PacketPusher(std::ostream& target)
        : target(target) {}
      virtual ~PacketPusher() {}
      /**
       * defined in cpp
       */
      virtual void visit(const hexabus::ErrorPacket& error);
      virtual void visit(const hexabus::QueryPacket& query);
      virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery);
      virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo);
      /** 
       * not needed.
       */
      virtual void visit(const hexabus::InfoPacket<bool>& info) { printValuePacket(info, "Bool"); }
      virtual void visit(const hexabus::InfoPacket<uint8_t>& info) { printValuePacket(info, "UInt8"); }
      virtual void visit(const hexabus::InfoPacket<uint32_t>& info) { printValuePacket(info, "UInt32"); }
      virtual void visit(const hexabus::InfoPacket<float>& info) { printValuePacket(info, "Float"); }
      virtual void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info) { printValuePacket(info, "Datetime"); }
      virtual void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info) { printValuePacket(info, "Timestamp"); }
      virtual void visit(const hexabus::InfoPacket<std::string>& info) { printValuePacket(info, "String"); }
      virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (16 bytes)"); }
      virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (66 bytes)"); }
      virtual void visit(const hexabus::WritePacket<bool>& write) {}
      virtual void visit(const hexabus::WritePacket<uint8_t>& write) {}
      virtual void visit(const hexabus::WritePacket<uint32_t>& write) {}
      virtual void visit(const hexabus::WritePacket<float>& write) {}
      virtual void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) {}
      virtual void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) {}
      virtual void visit(const hexabus::WritePacket<std::string>& write) {}
      virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
      virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
  };

}


#endif /* LIBHEXANODE_PACKET_PUSHER_HPP */

