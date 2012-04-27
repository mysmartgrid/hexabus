#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include "../../../shared/hexabus_packet.h"
#include "hexabridge.hpp"


// BroadcastListener implementation
BroadcastListener::BroadcastListener(boost::asio::io_service *io_service, PacketFilter *filter) {
			nic = new boost::asio::ip::udp::endpoint(boost::asio::ip::address_v6::any(), HXB_PORT);
			// Setting the socket to reuse the address. Is this a good idea?
			socket = new boost::asio::ip::udp::socket(*io_service, boost::asio::ip::udp::v6());
			boost::asio::socket_base::reuse_address addr(true);
			socket->set_option(addr);
			socket->bind(*nic);
			remoteEP = new boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v6(), HXB_PORT);
			pfilter = filter;
			startListening();
		}
	
BroadcastListener::~BroadcastListener() {
	socket->close();
	delete remoteEP;
	delete socket;
	delete nic;
}

void BroadcastListener::startListening() {
	socket->async_receive_from(boost::asio::buffer(recvBuffer), *remoteEP,
				boost::bind(&BroadcastListener::receiveHandler, this,
				boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void BroadcastListener::receiveHandler(const boost::system::error_code &error, std::size_t size) {
	// Check if received packet is a broadcast. In my opinion this is some kind of hack,
	// but the asio documentation keeps its silence on this matter. This Code will most likely _not_ work with Windows, 
	// which is okay, because it is supposed to run on an openwrt anyway.
	// Broadcast IP-Addresses are in the style of <IP>%receivingInterface. So if we check for "%" in the address,
	// we can determine wether it was a broadcast.
	std::string addr = remoteEP->address().to_string();
	if(addr.find("%") != std::string::npos) {
		std::cout << "[BCastListener] Received " << size << " bytes from " << remoteEP->address().to_string() << std::endl;
		pfilter->filterBroadcast(new std::vector<char>(recvBuffer, recvBuffer+size), *remoteEP);
	}
	startListening();
}


// BridgeEndpoint implementation
BridgeEndpoint::BridgeEndpoint(std::string address, boost::asio::io_service *io_service, int id) {
	resolver = new boost::asio::ip::udp::resolver(*io_service);
	nic = new boost::asio::ip::udp::endpoint();
	boost::asio::ip::udp::resolver::query query(address, boost::lexical_cast<std::string>(HXB_PORT+id));
	boost::asio::ip::udp::resolver::iterator iterator = resolver->resolve(query);
	*nic = iterator->endpoint();
	socket = new boost::asio::ip::udp::socket(*io_service, iterator->endpoint());
	//boost::asio::socket_base::broadcast bcast(true);									// Enable broadcasts
	//socket->set_option(bcast);
	i = id;
}
		
BridgeEndpoint::~BridgeEndpoint() {
	socket->close();
	delete nic;
	delete socket;
}

void BridgeEndpoint::sendBroadcast(std::vector<char> *data) {
	std::cout << "[BridgeEndpoint " << i << "] Received " << data->size() << " for sending" << std::endl;
	boost::asio::ip::udp::endpoint bcastEP(boost::asio::ip::address::from_string("ff02::1"), HXB_PORT);				// "Broadcast" Group
	socket->send_to(boost::asio::buffer(*data), bcastEP);
}

boost::asio::ip::udp::endpoint BridgeEndpoint::getEndpoint() {
	return *nic;
}

// PacketFilter implementation
PacketFilter::PacketFilter(std::vector<BridgeEndpoint*> endpointList) {
	endpoints = endpointList;
}
		
PacketFilter::~PacketFilter() {
	//delete endpoints;
}

void PacketFilter::filterBroadcast(std::vector<char> *data, boost::asio::ip::udp::endpoint remoteEP) {
	// Do not send our own packets again
	bool fromLocal = false;
	for(unsigned int i = 0;i < endpoints.size();i++) {
		if(remoteEP.address() == endpoints.at(i)->getEndpoint().address()) {
			fromLocal = true;
			break;
		}
	}

	if(!fromLocal) {
		std::cout << "[Filter] Send bcast on ALL the interfaces!" << std::endl;
		for(unsigned int i = 0;i < endpoints.size();i++) {
			endpoints.at(i)->sendBroadcast(data);
		}
	}
}

// Functions
void usage(const char *name) {
	std::cout << "Usage:" << std::endl;
	std::cout << name << " <IP of NIC1> <IP of NIC2> ..." << std::endl;
}

// Main
int main(int argc, char **argv) {
	if(argc < 2) {
		std::cout << "At least one argument is expected!" << std::endl;
		usage(argv[0]);
		return 1;
	}

	std::cout << "Hexabridge - A simple Hexabus Bridging Daemon." << std::endl;
	try {
		boost::asio::io_service *ioservice = new boost::asio::io_service();
		
		std::cout << "Initializing BridgeEndpoints..." << std::endl;
		
		std::vector<BridgeEndpoint*> endpoints(argc - 1, NULL);
		// TODO: Check the Input.
		for(int i = 0;i < argc - 1;i++) {
			endpoints.at(i) = new BridgeEndpoint(argv[i+1], ioservice, i+1);
		}
		
		std::cout << "Initializing PacketFilter..." << std::endl;
		PacketFilter *pfilter = new PacketFilter(endpoints);
		
		std::cout << "Initializing BroadcastListener..." << std::endl;
		BroadcastListener bListener(ioservice, pfilter);
		
		std::cout << "Done." << std::endl;
		ioservice->run();
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
