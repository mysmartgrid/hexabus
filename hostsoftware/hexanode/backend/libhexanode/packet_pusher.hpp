#ifndef LIBHEXANODE_PACKET_PUSHER_HPP
#define LIBHEXANODE_PACKET_PUSHER_HPP 1

#include <libhexanode/common.hpp>
#include <libhexanode/error.hpp>
#include <libhexanode/sensor_collection.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/device_interrogator.hpp>
#include <libhexabus/endpoint_registry.hpp>
#include <boost/network/protocol/http/client.hpp>
#include <boost/network/uri.hpp>
#include <sstream>
#include <map>
#include <set>

namespace hexanode {

  class PacketPusher : public hexabus::PacketVisitor {
    public:
      PacketPusher(hexabus::Socket* socket,
          hexanode::SensorStore::Ptr sensors,
          boost::network::http::client client,
          const boost::network::uri::uri& api_uri,
          std::ostream& target)
        : _info(*socket)
        , _sensors(sensors)
        , _client(client)
        , _api_uri(api_uri)
        , target(target) {}
      virtual ~PacketPusher() {}

			void push(const boost::asio::ip::udp::endpoint& endpoint, const hexabus::Packet& packet)
			{
				_endpoint = endpoint;
				visitPacket(packet);
			}

    private:
			hexabus::DeviceInterrogator _info;
			hexabus::EndpointRegistry _ep_registry;
      hexabus::Socket* _socket;
      boost::asio::ip::udp::endpoint _endpoint;
      hexanode::SensorStore::Ptr _sensors;
      boost::network::http::client _client;
      boost::network::uri::uri _api_uri;
      std::ostream& target;
			std::map<boost::asio::ip::address_v6, std::map<uint32_t, std::string> > _unidentified_devices;

			void deviceInfoReceived(const boost::asio::ip::address_v6& device, const hexabus::Packet& info);
			void deviceInfoError(const boost::asio::ip::address_v6& device, const hexabus::GenericException& error);

      void push_value(uint32_t eid, const std::string& value);

			template<typename T>
			void pushFromPacket(const hexabus::ValuePacket<T>& packet)
			{
				std::ostringstream oss;
				oss << std::fixed << std::setprecision(1) << packet.value();

				push_value(packet.eid(), oss.str());
			}

			void rejectPacket()
			{
				target << "Unexpected packet received" << std::endl;
			}


    public:
			/**
			 * Packets we are interested in
			 */
			virtual void visit(const hexabus::InfoPacket<bool>& info) { pushFromPacket(info); }
			virtual void visit(const hexabus::InfoPacket<uint8_t>& info) { pushFromPacket(info); }
			virtual void visit(const hexabus::InfoPacket<uint32_t>& info) { pushFromPacket(info); }
			virtual void visit(const hexabus::InfoPacket<float>& info) { pushFromPacket(info); }
			/**
			 * Ignored
			 */
			virtual void visit(const hexabus::ErrorPacket& error) { rejectPacket(); }
			virtual void visit(const hexabus::QueryPacket& query) { rejectPacket(); }
			virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) { rejectPacket(); }
			virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo) { rejectPacket(); }
			virtual void visit(const hexabus::InfoPacket<boost::posix_time::ptime>& info) { rejectPacket(); }
			virtual void visit(const hexabus::InfoPacket<boost::posix_time::time_duration>& info) { rejectPacket(); }
			virtual void visit(const hexabus::InfoPacket<std::string>& info) { rejectPacket(); }
			virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { rejectPacket(); }
			virtual void visit(const hexabus::InfoPacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& info) { rejectPacket(); }
			/** 
			 * not needed.
			 */
			virtual void visit(const hexabus::WritePacket<bool>& write) { rejectPacket(); }
			virtual void visit(const hexabus::WritePacket<uint8_t>& write) { rejectPacket(); }
			virtual void visit(const hexabus::WritePacket<uint32_t>& write) { rejectPacket(); }
			virtual void visit(const hexabus::WritePacket<float>& write) { rejectPacket(); }
			virtual void visit(const hexabus::WritePacket<boost::posix_time::ptime>& write) { rejectPacket(); }
			virtual void visit(const hexabus::WritePacket<boost::posix_time::time_duration>& write) { rejectPacket(); }
			virtual void visit(const hexabus::WritePacket<std::string>& write) { rejectPacket(); }
			virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_16BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) { rejectPacket(); }
			virtual void visit(const hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >& write) { rejectPacket(); }
  };

}


#endif /* LIBHEXANODE_PACKET_PUSHER_HPP */

