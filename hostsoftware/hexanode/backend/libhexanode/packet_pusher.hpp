#ifndef LIBHEXANODE_PACKET_PUSHER_HPP
#define LIBHEXANODE_PACKET_PUSHER_HPP 1

#include <libhexanode/common.hpp>
#include <libhexanode/error.hpp>
#include <libhexanode/sensor_collection.hpp>
#include <libhexabus/packet.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>

namespace hexanode {

  class PacketPusher : public hexabus::PacketVisitor {
    private:
      boost::asio::ip::udp::endpoint _endpoint;
      hexanode::SensorStore::Ptr _sensors;
      boost::network::http::client _client;
      boost::network::uri::uri _api_uri;
      std::ostream& target;

      void printValueHeader(uint32_t eid, const char* datatypeStr);


      template<typename T>
        void printValuePacket(const hexabus::ValuePacket<T>& packet, const char* datatypeStr)
        {
          printValueHeader(packet.eid(), datatypeStr);
          target << "Value:\t" << packet.value() << std::endl;
          target << "foooo" << std::endl;
          target << std::endl;
        }
      
        void printValuePacket (const hexabus::ValuePacket<uint32_t>& packet, const char* datatypeStr)
        {
          target << "USCHI!";
          printValueHeader(packet.eid(), datatypeStr);
          target << "Value:\t" << packet.value() << std::endl;
        }

        void printValuePacket (const hexabus::InfoPacket<float>& packet, const char* datatypeStr)
        {
          printValueHeader(packet.eid(), datatypeStr);
          target << "Value:\t" << packet.value() << std::endl;

          double min_value = 15;
          double max_value = 30;
          std::ostringstream oss;
          oss << _endpoint << "%" << packet.eid();
          std::string sensor_id=oss.str();
          hexanode::Sensor::Ptr sensor;
          try {
            hexanode::Sensor::Ptr find_sensor=_sensors->get_by_id(sensor_id);
            sensor=find_sensor;
          } catch (const hexanode::NotFoundException& e) {
            std::cout << "No information regarding sensor " << sensor_id << " found - creating sensor with boilerplate data." << std::endl;
            hexanode::Sensor::Ptr new_sensor(new hexanode::Sensor(sensor_id, sensor_id, min_value, max_value));
            _sensors->add_sensor(new_sensor);
            sensor=new_sensor;
          }
          try {
            sensor->post_value(_client, _api_uri, packet.value());
            std::cout << "Sensor " << sensor_id << ": submitted value " << packet.value() << std::endl;
          } catch (const std::exception& e) {
            std::cout << "Cannot submit sensor values (" << e.what() << ")" << std::endl;
            std::cout << "Attempting to re-create sensors." << std::endl;
            sensor->put(_client, _api_uri, 0.0); 
            sensor->post_value(_client, _api_uri, packet.value());
          }
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
      PacketPusher( const boost::asio::ip::udp::endpoint& endpoint,
          hexanode::SensorStore::Ptr sensors,
          boost::network::http::client client,
          const boost::network::uri::uri& api_uri,
          std::ostream& target)
        : _endpoint(endpoint)
          , _sensors(sensors)
          , _client(client)
          , _api_uri(api_uri)
          , target(target) {}
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

