#include <iostream>
#include <fstream>
#include <string.h>
#include <libhexabus/common.hpp>
#include <libhexabus/crc.hpp>
#include <time.h>
#include <libhexabus/packet.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/error.hpp>
#include <algorithm>
#include <vector>
#include <typeinfo>
namespace po = boost::program_options;

#include "../../../shared/hexabus_packet.h"
#include "../../../shared/hexabus_definitions.h"
#include "../../../shared/hexabus_statemachine_structs.h"
#include "../../../shared/endpoints.h"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

enum ErrorCode {
	ERR_NONE = 0,

	ERR_UNKNOWN_PARAMETER = 1,
	ERR_PARAMETER_MISSING = 2,
	ERR_PARAMETER_FORMAT = 3,
	ERR_PARAMETER_VALUE_INVALID = 4,
	ERR_NETWORK = 5,

	ERR_INVALID_BINARY = 6,
	ERR_SM_OP = 7,
	ERR_UPLOAD_FAILED = 8,
	ERR_READ_FAILED = 9,

	ERR_OTHER = 127
};

ErrorCode assert_statemachine_state(hexabus::Socket& network, const boost::asio::ip::address_v6& dest, STM_state_t req_state)
{
	network.send(hexabus::WritePacket<uint8_t>(EP_SM_CONTROL, req_state), dest);
	network.send(hexabus::QueryPacket(EP_SM_CONTROL), dest);

	while (true) {
		std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> pair;
		try {
			namespace hf = hexabus::filtering;

			pair = network.receive(hf::eid() == EP_SM_CONTROL && hf::sourceIP() == dest);
		} catch (const hexabus::NetworkException& e) {
			std::cerr << "Error receiving packet: " << e.code().message() << std::endl;
			return ERR_NETWORK;
		} catch (const hexabus::GenericException& e) {
			std::cerr << "Error receiving packet: " << e.what() << std::endl;
			return ERR_NETWORK;
		}

		const hexabus::InfoPacket<uint8_t>* u8 = dynamic_cast<const hexabus::InfoPacket<uint8_t>*>(pair.first.get());

		if (u8) {
			switch (req_state) {
				case STM_STATE_STOPPED:
					if (u8->value() == STM_STATE_STOPPED) {
						std::cout << "State machine has been stopped successfully" << std::endl;
					} else {
						std::cerr << "Failed to stop state machine - aborting." << std::endl;
						return ERR_NETWORK;
					}
					break;

				case STM_STATE_RUNNING:
					if (u8->value() == STM_STATE_RUNNING) {
						std::cout << "State machine is running." << std::endl;
					} else {
						std::cerr << "Failed to start state machine - aborting." << std::endl;
						return ERR_NETWORK;
					}
					break;

				default:
					std::cout << "Unexpected STM_STATE requested - aborting." << std::endl;
					return ERR_NETWORK;
					break;
			}
		} else {
			std::cout << "Expected uint8 data in packet - got something different." << std::endl;
			return ERR_NETWORK;
		}
		break;
	}

	return ERR_NONE;
}

static const size_t UploadChunkSize = EE_STATEMACHINE_CHUNK_SIZE;

class ChunkSender {
	private:
		boost::asio::deadline_timer timeout;
		hexabus::Socket& socket;
		boost::asio::ip::address_v6 target;
		int retryLimit;
		boost::signals2::connection replyHandler;

		uint8_t chunkId;
		ErrorCode result;

		int failCount;

		void armTimer()
		{
			timeout.expires_from_now(boost::posix_time::seconds(1));
		}

		void failOne()
		{
			failCount++;
			if (failCount >= retryLimit || retryLimit < 0) {
				std::cout << std::endl;
				result = ERR_UPLOAD_FAILED;
				socket.ioService().stop();
			}
		}

		void onReply(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& from)
		{
			if (dynamic_cast<const hexabus::InfoPacket<bool>&>(packet).value()) {
				result = ERR_NONE;
				socket.ioService().stop();
			} else {
				std::cout << "F" << std::flush;
				failOne();
			}
		}

		void onTimeout(const boost::system::error_code& err)
		{
			std::cout << "T" << std::flush;
			failOne();
			armTimer();
		}

	public:
		ChunkSender(hexabus::Socket& socket, const boost::asio::ip::address_v6& target, int retryLimit)
			: timeout(socket.ioService()), socket(socket), target(target), retryLimit(retryLimit)
		{
			timeout.async_wait(boost::bind(&ChunkSender::onTimeout, this, _1));

			namespace hf = hexabus::filtering;

			replyHandler = socket.onPacketReceived(
					boost::bind(&ChunkSender::onReply, this, _1, _2),
					hf::sourceIP() == target && hf::isInfo<bool>() && hf::eid() == EP_SM_UP_ACKNAK);
		}

		~ChunkSender()
		{
			replyHandler.disconnect();
		}

		ErrorCode sendChunk(uint8_t chunkId, const boost::array<uint8_t, UploadChunkSize>& chunk)
		{
			boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> packet_data;

			this->chunkId = chunkId;
			packet_data[0] = chunkId;
			std::copy(chunk.begin(), chunk.end(), packet_data.begin() + 1);

			socket.send(hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >(EP_SM_UP_RECEIVER, packet_data), target);

			armTimer();
			failCount = 0;

			socket.ioService().reset();
			socket.ioService().run();

			return result;
		}
};

int main(int argc, char** argv) {
	std::ostringstream oss;
	oss << "Usage: " << argv[0] << " IP [additional options] ACTION";
	po::options_description desc(oss.str());
	desc.add_options()
		("help,h", "produce help message")
		("version", "print libhexabus version and exit")
		("ip,i", po::value<std::string>(), "the hostname to connect to. If this option is not set, the target IP address from the program file will be used.")
		("program,p", po::value<std::string>(), "the state machine program to be uploaded")
		("retry,r", po::value<int>(), "number of retries for failed/timed out uploads")
		;
	po::positional_options_description p;
	p.add("program", 1);
	p.add("ip", 1);
	po::variables_map vm;

	// Begin processing of commandline parameters.
	try {
		po::store(po::command_line_parser(argc, argv).
				options(desc).positional(p).run(), vm);
		po::notify(vm);
	} catch (std::exception& e) {
		std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
		return ERR_UNKNOWN_PARAMETER;
	}

	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return 0;
	}

	if (vm.count("version")) {
		std::cout << "hexaupload -- hexabus state machine upload utility" << std::endl;
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		return 0;
	}

	if (!vm.count("program")) {
		std::cout << "Cannot proceed without a program (-p <FILE>)" << std::endl;
		return ERR_PARAMETER_MISSING;
	}
	if (!vm.count("ip")) {
		std::cout << "Cannot proceed without IP of the target device (-i <IP>)" << std::endl;
		return ERR_PARAMETER_MISSING;
	}

	std::basic_ifstream<uint8_t> in(vm["program"].as<std::string>().c_str(),
			std::ios_base::in | std::ios::ate | std::ios::binary);
	if (!in) {
		std::cerr << "Error: Could not open input file: "
			<< vm["program"].as<std::string>() << std::endl;
		return ERR_PARAMETER_VALUE_INVALID;
	}
	in.unsetf(std::ios::skipws); // No white space skipping!

	size_t size = in.tellg();
	in.seekg(0, std::ios::beg);

	boost::asio::ip::address_v6 target;

	// first, read target IP
	boost::asio::ip::address_v6::bytes_type ipBuffer;
	in.read(ipBuffer.c_array(), ipBuffer.size());

	if (vm.count("ip")) {
		boost::system::error_code err;

		target = boost::asio::ip::address_v6::from_string(vm["ip"].as<std::string>());
		if (err) {
			std::cerr << vm["ip"].as<std::string>() << " is not a valid IP address" << std::endl;
			return ERR_PARAMETER_FORMAT;
		}

		std::cout << "Using target IP address from program file: " << target << std::endl;
	} else {
		target = boost::asio::ip::address_v6(ipBuffer);
	}

	// then do the actual programming
	if ((size - in.tellg()) % UploadChunkSize != 0) {
		std::cerr << "State machine binary is invalid" << std::endl;
		return ERR_INVALID_BINARY;
	}
	std::cout << "Uploading program, size=" << (size - in.tellg()) << std::endl;

	boost::asio::io_service io;
	try {
		hexabus::Socket socket(io);
		uint8_t chunkId = 0;
		int retryLimit = vm.count("retry") ? vm["retry"].as<int>() : 3;
		ChunkSender sender(socket, target, retryLimit);
		ErrorCode err;

		if ((err = assert_statemachine_state(socket, target, STM_STATE_STOPPED))) {
			return err;
		}

		/**
		 * for all bytes in the binary format:
		 * 0. generate the next 64 byte chunk, failure counter:=0
		 * 1. prepend chunk ID
		 * 2. send to hexabus device
		 * 3. wait for ACK/NAK:
		 *    - if ACK, next chunk
		 *    - if NAK: Retransmit current packet. failure counter++. Abort if
		 *    maxtry reached.
		 */
		while (in && !in.eof()) {
			boost::array<uint8_t, UploadChunkSize> chunk;

			in.read(chunk.c_array(), chunk.size());
			err = sender.sendChunk(chunkId, chunk);

			if (err) {
				std::cerr << "Failed to upload program" << std::endl;
				return err;
			}

			chunkId++;
		}
		if (!in) {
			std::cerr << "Can't read program" << std::endl;
			return ERR_READ_FAILED;
		}

		if ((err = assert_statemachine_state(socket, target, STM_STATE_RUNNING))) {
			return err;
		}
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Network error: " << e.code().message() << std::endl;
		return ERR_NETWORK;
	}

	std::cout << "Program uploaded successfully." << std::endl;

	return ERR_NONE;
}

