#include <iostream>
#include <string.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "../../../shared/hexabus_packet.h"
#include <libhexabus/packet.hpp>

const int maxPacketLength = 1024;		// TODO: Exact definition in hexabus_packet.h??
class networkEndpoint {
	public:
		networkEndpoint(const char *address, boost::asio::io_service *io_service) {
			nic = new boost::asio::ip::udp::endpoint(boost::asio::ip::address::from_string(address), HXB_PORT);
			socket = new boost::asio::ip::udp::socket(*io_service, *nic);
			startListening();
		}
	
	private:
		boost::asio::io_service *io_service;
		boost::asio::ip::udp::socket *socket;
		boost::asio::ip::udp::endpoint *nic;
		char recvBuffer[maxPacketLength];
		void startListening() {
			socket->async_receive(boost::asio::buffer(recvBuffer), boost::bind(&networkEndpoint::receiveHandler, this,
						boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}

		void receiveHandler(const boost::system::error_code &error, std::size_t size) {
			std::cout << "Received " << size << " bytes." << std::endl;
			hexabus::PacketHandling phandling(recvBuffer);
			std::cout << "PType: " << (int)phandling.getPacketType() << " Datatype: " 
				<< (int)phandling.getDatatype() << " EID: " << (int)phandling.getEID() << std::endl;
			startListening();
		}
};

int main(int argc, char **argv) {
	std::cout << "Hexabridge - A simple Hexabus Bridging Daemon." << std::endl;
	try {
		boost::asio::io_service *ioservice = new boost::asio::io_service();
		networkEndpoint ep("aaaa::1", ioservice);
		ioservice->run();
		std::cout << "Issued run, service should be running now" << std::endl;
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
