#ifndef LIBHEXANODE_PACKET_PUSHER_HPP
#define LIBHEXANODE_PACKET_PUSHER_HPP 1

#include <libhexanode/common.hpp>
#include <libhexanode/error.hpp>
#include <libhexanode/sensor_collection.hpp>
#include <libhexanode/info_query.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>
#include <sstream>

namespace hexanode {

  class PacketPusher : public hexabus::PacketVisitor {
    public:
      PacketPusher(hexabus::Socket* socket,
          const boost::asio::ip::udp::endpoint& endpoint,
          hexanode::SensorStore::Ptr sensors,
          boost::network::http::client client,
          const boost::network::uri::uri& api_uri,
          std::ostream& target)
        : _info(new hexanode::InfoQuery(socket))
        , _endpoint(endpoint)
        , _sensors(sensors)
        , _client(client)
        , _api_uri(api_uri)
        , target(target) {}
      virtual ~PacketPusher() {}


    private:
      hexanode::InfoQuery::Ptr _info;
      hexabus::Socket* _socket;
      boost::asio::ip::udp::endpoint _endpoint;
      hexanode::SensorStore::Ptr _sensors;
      boost::network::http::client _client;
      boost::network::uri::uri _api_uri;
      std::ostream& target;

      void printValueHeader(uint32_t eid, const char* datatypeStr);

      template<typename T> 
      std::string numeric2string(T value) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << value;
        return oss.str();
      }

      void push_value(uint32_t eid, const std::string& value);

      template<typename T>
        void printValuePacket(const hexabus::ValuePacket<T>& packet, const char* datatypeStr)
        {
          printValueHeader(packet.eid(), datatypeStr);
          target << " Value:\t" << packet.value() << std::endl;
        }

      void printValuePacket (const hexabus::ValuePacket<uint32_t>& packet, const char* datatypeStr)
      {
        printValueHeader(packet.eid(), datatypeStr);
        target << " Value:\t" << packet.value() << std::endl;
        push_value(packet.eid(), numeric2string(packet.value()));
      }

      void printValuePacket (const hexabus::InfoPacket<float>& packet, const char* datatypeStr)
      {
        printValueHeader(packet.eid(), datatypeStr);
        target << " Value:\t" << packet.value() << std::endl;
        push_value(packet.eid(), numeric2string(packet.value()));
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
      virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (16 bytes)"); }
      virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { printValuePacket(info, "Binary (66 bytes)"); }
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
      virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
      virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) {}
  };

}


#endif /* LIBHEXANODE_PACKET_PUSHER_HPP */

