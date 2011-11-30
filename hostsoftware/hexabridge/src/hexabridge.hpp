#ifndef HEXABRIDGE_HPP
#define HEXABRIDGE_HPP 1

const int maxPacketLength = 1024;		// TODO: Exact definition in hexabus_packet.h??

class BridgeEndpoint {
	public:
		BridgeEndpoint(boost::asio::ip::address address, boost::asio::io_service *io_service, int id);
		~BridgeEndpoint();
		void sendBroadcast(std::vector<char> *data);
		boost::asio::ip::udp::endpoint getEndpoint();
	private:
		int i;
		boost::asio::io_service *io_service;
		boost::asio::ip::udp::endpoint *nic;
		boost::asio::ip::udp::socket *socket;
};


class PacketFilter {
	public:
		PacketFilter(std::vector<BridgeEndpoint*> endpointList); 
		~PacketFilter();
		void filterBroadcast(std::vector<char> *data, boost::asio::ip::udp::endpoint remoteEP);

	private:
		std::vector<BridgeEndpoint*> endpoints;
};

class BroadcastListener {
	public:
		BroadcastListener(boost::asio::io_service *io_service, PacketFilter *filter);
		~BroadcastListener();

	private:
		char recvBuffer[maxPacketLength];
		void startListening();
		void receiveHandler(const boost::system::error_code &error, std::size_t size);
		
		PacketFilter *pfilter;
		boost::asio::ip::udp::endpoint *remoteEP;
		boost::asio::io_service *io_service;
		boost::asio::ip::udp::socket *socket;
		boost::asio::ip::udp::endpoint *nic;
};

#endif
