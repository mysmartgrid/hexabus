#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/foreach.hpp>
#include <libhexabus/common.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/endpoint_registry.hpp>

#include "../../../shared/endpoints.h"
#include "../../../shared/hexabus_definitions.h"
#include "../../../shared/hexabus_types.h"

#ifdef DEBUG
int64_t failcnt = 0;
int64_t inforcvd = 0;
#endif

namespace po = boost::program_options;

using namespace hexabus;

bool verbose = false;


boost::asio::ip::address_v6 hxb_broadcast_address = boost::asio::ip::address_v6::from_string(HXB_GROUP);

struct ErrorCallback {
	void operator()(const hexabus::GenericException& error)
	{
		std::cerr << "Error receiving packet: " << error.what() << std::endl;
		exit(1);
	}
};

class PacketSender {
private:
	hexabus::Socket* network;
	hexabus::Socket* reset_network;
	std::set<boost::asio::ip::address_v6> addresses;

	std::queue<Packet::Ptr> distribution_queue;
	std::set<boost::asio::ip::address_v6> remaining_addresses;

	bool ongoing_transmission;

	void doReset() {
		ongoing_transmission = false;

		remaining_addresses.clear();
		std::queue<Packet::Ptr> empty;
		std::swap(distribution_queue, empty);

		distribution_queue.push(Packet::Ptr(new WritePacket<uint8_t>(EP_SM_CONTROL, STM_STATE_RUNNING, HXB_FLAG_WANT_ACK|HXB_FLAG_RELIABLE)));

		if(verbose)
			std::cout << "Broadcasting reset..." << std::endl;

		send();
	}

public:
	PacketSender(hexabus::Socket* network, hexabus::Socket* reset_network) {
		this->network = network;
		this->reset_network = reset_network;
		this->ongoing_transmission = false;
	}

	void transmissionCallback(const hexabus::Packet& packet, uint16_t seqNum, const boost::asio::ip::udp::endpoint asio_ep, bool failed) {
		if(failed) {

			#ifdef DEBUG
			failcnt++;
			#endif

			if(verbose) {
				std::cout << "Info distribution failed! " << std::endl;

				std::cout << "Missing ACKs from:" << std::endl;
				for(std::set<boost::asio::ip::address_v6>::iterator it = remaining_addresses.begin(); it != remaining_addresses.end(); ++it)
				{
					std::cout << "\t" << it->to_string() << std::endl;
				}
			}

			doReset();

		} else {
			if(verbose)
				std::cout << "Info distribution successful!" << std::endl;

			if(!distribution_queue.empty())
				distribution_queue.pop();

			ongoing_transmission = false;
			remaining_addresses = std::set<boost::asio::ip::address_v6>(addresses);

			send();
		}

		#ifdef DEBUG
		std::cout << "Failed distributions: " << failcnt << std::endl;
		#endif
	}

	void resetCallback(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep) {

		if(verbose)
			std::cout << "Received reset request from " << asio_ep.address().to_v6().to_string() << std::endl;

		reset_network->send(AckPacket(packet.sequenceNumber()), asio_ep);

		doReset();
	}

	void ackCallback(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep) {
		const hexabus::AckPacket* ack = dynamic_cast<const hexabus::AckPacket* >(&packet);

		if(ack == NULL)
			return;

		Socket::Association& assoc = network->getAssociation(boost::asio::ip::udp::endpoint(hxb_broadcast_address, 61616));

		if(ack->cause() == assoc.want_ack_for) {
			if(verbose)
				std::cout << "Received ACK for " << ack->cause() << " from " << asio_ep.address().to_v6().to_string() << std::endl;
			remaining_addresses.erase(asio_ep.address().to_v6());
		}

		if(remaining_addresses.empty()) {
			if(verbose)
				std::cout << "Received all ACKs." << std::endl;
			network->upperLayerAckReceived(boost::asio::ip::udp::endpoint(hxb_broadcast_address, 61616) , assoc.want_ack_for);
		}
	}

	void infoCallback(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint asio_ep) {

		if(packet.flags()&HXB_FLAG_RELIABLE) {

			if(verbose) {
				std::cout << "Received Info packet " << packet.sequenceNumber()
									<< " from " << asio_ep.address().to_v6().to_string() << std::endl;

			}

			#ifdef DEBUG
				inforcvd++;
				std::cout << "Total info packets received: " << inforcvd << std::endl;
			#endif

			addresses.insert(asio_ep.address().to_v6());

			network->sendUpperLayerAck(boost::asio::ip::udp::endpoint(hxb_broadcast_address, 61616), packet.sequenceNumber());

			do {
				const InfoPacket<bool>* info_b = dynamic_cast<const InfoPacket<bool>* >(&packet);
				if(info_b !=NULL) {
					ProxyInfoPacket<bool> pinfo(asio_ep.address().to_v6(), info_b->eid(), info_b->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<uint8_t>* info_u8 = dynamic_cast<const InfoPacket<uint8_t>* >(&packet);
				if(info_u8 !=NULL) {
					ProxyInfoPacket<uint8_t> pinfo(asio_ep.address().to_v6(), info_u8->eid(), info_u8->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<uint32_t>* info_u32 = dynamic_cast<const InfoPacket<uint32_t>* >(&packet);
				if(info_u32 !=NULL) {
					ProxyInfoPacket<uint32_t> pinfo(asio_ep.address().to_v6(), info_u32->eid(), info_u32->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<uint64_t>* info_u64 = dynamic_cast<const InfoPacket<uint64_t>* >(&packet);
				if(info_u64 !=NULL) {
					ProxyInfoPacket<uint64_t> pinfo(asio_ep.address().to_v6(), info_u64->eid(), info_u64->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<int8_t>* info_s8 = dynamic_cast<const InfoPacket<int8_t>* >(&packet);
				if(info_s8 !=NULL) {
					ProxyInfoPacket<int8_t> pinfo(asio_ep.address().to_v6(), info_s8->eid(), info_s8->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<int32_t>* info_s32 = dynamic_cast<const InfoPacket<int32_t>* >(&packet);
				if(info_s32 !=NULL) {
					ProxyInfoPacket<int32_t> pinfo(asio_ep.address().to_v6(), info_s32->eid(), info_s32->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<int64_t>* info_s64 = dynamic_cast<const InfoPacket<int64_t>* >(&packet);
				if(info_s64 !=NULL) {
					ProxyInfoPacket<uint64_t> pinfo(asio_ep.address().to_v6(), info_s64->eid(), info_s64->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<float>* info_f = dynamic_cast<const InfoPacket<float>* >(&packet);
				if(info_f !=NULL) {
					ProxyInfoPacket<float> pinfo(asio_ep.address().to_v6(), info_f->eid(), info_f->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<std::string>* info_s = dynamic_cast<const InfoPacket<std::string>* >(&packet);
				if(info_s !=NULL) {
					ProxyInfoPacket<std::string> pinfo(asio_ep.address().to_v6(), info_s->eid(), info_s->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<std::array<uint8_t, 16> >* info_16b = dynamic_cast<const InfoPacket<std::array<uint8_t, 16> >* >(&packet);
				if(info_16b !=NULL) {
					ProxyInfoPacket<std::array<uint8_t, 16> > pinfo(asio_ep.address().to_v6(), info_16b->eid(), info_16b->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
				const InfoPacket<std::array<uint8_t, 65> >* info_65b = dynamic_cast<const InfoPacket<std::array<uint8_t, 65> >* >(&packet);
				if(info_65b !=NULL) {
					ProxyInfoPacket<std::array<uint8_t, 65> > pinfo(asio_ep.address().to_v6(), info_65b->eid(), info_65b->value(), HXB_FLAG_WANT_UL_ACK|HXB_FLAG_RELIABLE);
					distribution_queue.push(Packet::Ptr(pinfo.clone()));
					break;
				}
			} while(false);

			send();
		}
	}

	void send() {

		if(!ongoing_transmission && !distribution_queue.empty()) {
			if(verbose)
				std::cout << "Sending Pinfo..." << std::endl;
			ongoing_transmission = true;
			network->onPacketTransmitted(boost::bind(&PacketSender::transmissionCallback, this, _1, _2, _3, _4),
				*(distribution_queue.front()), boost::asio::ip::udp::endpoint(hxb_broadcast_address, 61616));
		}
	}
};

int main(int argc, char** argv)
{
	// the command line interface
	std::ostringstream oss;
	oss << "Usage: " << argv[0] << " [additional options]";
	po::options_description desc(oss.str());
	desc.add_options()
		("help,h", "produce help message")
		("version", "print version and exit")
		("interface,I", po::value<std::string>(), "Interface to send multicast from")
		("ip,i", po::value<std::string>(), "IP address to listen on")
		("verbose,V", "print more status information")
		;

	po::positional_options_description p;

	po::variables_map vm;

	// begin processing of command line parameters
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
		po::notify(vm);
	} catch (std::exception& e) {
		std::cerr << "Cannot process command line: " << e.what() << std::endl;
		exit(-1);
	}

	if(vm.count("help"))
	{
		std::cout << desc << std::endl;
		return 1;
	}

	if(vm.count("version"))
	{
		std::cout << "hexaproxyd -- hexabus reliable multicast daemon" << std::endl;
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		return 0;
	}

	if(vm.count("verbose"))
		verbose = true;

	boost::asio::io_service io;
	hexabus::Socket* network = new hexabus::Socket(io, 10);
	hexabus::Socket* reset_network = new hexabus::Socket(io, 10);
	hexabus::Listener* listener = new hexabus::Listener(io);

	boost::asio::ip::address_v6 bind_addr(boost::asio::ip::address_v6::any());

	try {
		if(vm.count("interface")) {
			if(verbose)
				std::cout << "Using network interface " << vm["interface"].as<std::string>() << "." << std::endl;
			network->mcast_from(vm["interface"].as<std::string>());
		} else {
			std::cerr << "Please specify interface to send multicasts from." << std::endl;
			return 1;
		}
	} catch(const hexabus::NetworkException& e) {
		std::cerr << "Could not open socket: " << e.code().message() << std::endl;
		return 1;
	}

	if(!vm.count("ip")) {
		std::cerr << "Please specific an ip address to listen on." << std::endl;
		return 1;
	}

	network->bind(bind_addr);
	reset_network->bind(boost::asio::ip::udp::endpoint(
		boost::asio::ip::address_v6::from_string(vm["ip"].as<std::string>()),
		61616
	));

	try {
		listener->listen(vm["interface"].as<std::string>());
	} catch(const hexabus::NetworkException& e) {
		std::cerr << "Could bind to multicast address: " << e.code().message() << std::endl;
		return 1;
	}


	PacketSender sender(network, reset_network);
	ErrorCallback errorCallback;

	boost::signals2::connection c1 = listener->onAsyncError(errorCallback);
	boost::signals2::connection c2 = listener->onPacketReceived(boost::bind(&PacketSender::infoCallback, &sender, _1, _2),
		filtering::isInfo<bool>() ||
		filtering::isInfo<uint8_t>() ||
		filtering::isInfo<uint16_t>() ||
		filtering::isInfo<uint32_t>() ||
		filtering::isInfo<uint64_t>() ||
		filtering::isInfo<int8_t>() ||
		filtering::isInfo<int16_t>() ||
		filtering::isInfo<int32_t>() ||
		filtering::isInfo<int64_t>() ||
		filtering::isInfo<float>() ||
		filtering::isInfo<std::string>() ||
		filtering::isInfo<std::array<uint8_t, 16> >() ||
		filtering::isInfo<std::array<uint8_t, 65> >());
	boost::signals2::connection c3 = network->onPacketReceived(boost::bind(&PacketSender::ackCallback, &sender, _1, _2), filtering::isAck());
	boost::signals2::connection c4 = reset_network->onPacketReceived(boost::bind(&PacketSender::resetCallback, &sender, _1, _2),
		filtering::isWrite<uint8_t>() &&
		filtering::eid() == EP_SM_CONTROL);

	if(verbose) {
		std::cout << "Listening for info packets..." << std::endl;
	}

	network->ioService().reset();
	network->ioService().run();

	c1.disconnect();
	c2.disconnect();
	c3.disconnect();
	c4.disconnect();
}

