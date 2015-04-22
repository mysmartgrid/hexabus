#include <iostream>
#include <fstream>
#include <string.h>
#include <libhexabus/common.hpp>
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

#include "../../../shared/hexabus_definitions.h"
#include "../../../shared/endpoints.h"
#include "shared.hpp"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

using namespace hexabus;

enum ErrorCode {
	ERR_NONE = 0,

	ERR_UNKNOWN_PARAMETER = 1,
	ERR_PARAMETER_MISSING = 2,
	ERR_PARAMETER_FORMAT = 3,
	ERR_PARAMETER_VALUE_INVALID = 4,
	ERR_NETWORK = 5,

	ERR_INVALID_BINARY = 6,
	ERR_SM_OP_FAILED = 7,
	ERR_MAXRETRY_EXCEEDED = 8,
	ERR_READ_FAILED = 9,
	ERR_PARAMETER_INVALID = 10,

	ERR_OTHER = 127
};

class RetryingPacketSender {
	protected:
		hexabus::Socket& socket;
		boost::asio::ip::address_v6 target;

		ErrorCode result;

		void finish(ErrorCode result)
		{
			this->result = result;
			socket.ioService().stop();
		}

		void onTransmitted(const Packet& packet, uint16_t seqNum, const boost::asio::ip::udp::endpoint& from, bool transmissionFailed)
		{
			if (!transmissionFailed) {
				std::cout << "." << std::flush;
				finish(ERR_NONE);
			} else {
				finish(ERR_MAXRETRY_EXCEEDED);
			}
		}

	public:
		RetryingPacketSender(hexabus::Socket& socket, const boost::asio::ip::address_v6& target)
			: socket(socket), target(target)
		{
		}

		ErrorCode send(const Packet& packet)
		{
			socket.onPacketTransmitted(boost::bind(&RetryingPacketSender::onTransmitted, this, _1, _2, _3, _4),
				packet, boost::asio::ip::udp::endpoint(target, 61616));

			socket.ioService().reset();
			socket.ioService().run();

			return result;
		}
};

class RemoteStateMachine : protected RetryingPacketSender {
	private:
		STM_state_t reqState;

		ErrorCode setAndCheckState(STM_state_t state)
		{
			reqState = state;
			ErrorCode err = send(hexabus::WritePacket<uint8_t>(EP_SM_CONTROL, state, HXB_FLAG_WANT_ACK|HXB_FLAG_RELIABLE));

			if (err) {
				std::cout << std::endl
					<< "Failed to " << (state == STM_STATE_RUNNING ? "start" : "stop") << " state machine - aborting." << std::endl;
			} else {
				switch (reqState) {
					case STM_STATE_STOPPED:
						std::cout << "State machine stopped" << std::endl;
						break;
					case STM_STATE_RUNNING:
						std::cout << "State machine is running." << std::endl;
						break;
				}
			}
			return err;
		}

	public:
		RemoteStateMachine(hexabus::Socket& socket, const boost::asio::ip::address_v6& target)
			: RetryingPacketSender(socket, target)
		{
		}

		ErrorCode start()
		{
			return setAndCheckState(STM_STATE_RUNNING);
		}

		ErrorCode stop()
		{
			return setAndCheckState(STM_STATE_STOPPED);
		}
};

static const size_t UploadChunkSize = 64;

class ChunkSender : protected RetryingPacketSender {
	public:
		ChunkSender(hexabus::Socket& socket, const boost::asio::ip::address_v6& target)
			: RetryingPacketSender(socket, target)
		{}

		ErrorCode sendChunk(uint8_t chunkId, const std::array<uint8_t, UploadChunkSize>& chunk)
		{
			std::array<uint8_t, 65> packet_data;

			packet_data[0] = chunkId;
			std::copy(chunk.begin(), chunk.end(), packet_data.begin() + 1);

			return send(hexabus::WritePacket<std::array<uint8_t, 65> >(EP_SM_UP_RECEIVER, packet_data, HXB_FLAG_WANT_ACK|HXB_FLAG_RELIABLE));
		}
};

int main(int argc, char** argv) {
	std::ostringstream oss;
	oss << "Usage: " << argv[0] << " IP [additional options] ACTION";
	po::options_description desc(oss.str());
	desc.add_options()
		("help,h", "produce help message")
		("version", "print libhexabus version and exit")
		("ip,i", po::value<std::string>(), "the hostname to connect to")
		("program,p", po::value<std::string>(), "the state machine program to be uploaded")
		("retry,r", po::value<int>(), "number of retries for failed/timed out uploads")
		("clear,c", "delete the device's state machine")
		;
	po::positional_options_description p;
	po::variables_map vm;

	const size_t PROG_DEFAULT_LENGTH = 1600;

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
		return ERR_NONE;
	}

	if (vm.count("version")) {
		std::cout << "hexaupload -- hexabus state machine upload utility" << std::endl;
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		return ERR_NONE;
	}

	switch (vm.count("program") + vm.count("clear")) {
		case 0:
			std::cout << "No action specified";
			return ERR_PARAMETER_INVALID;

		case 1:
			break;

		default:
			std::cout << "Only one of --program and --clear may be given" << std::endl;
			return ERR_PARAMETER_INVALID;
	}

	if (!vm.count("ip")) {
		std::cout << "Cannot proceed without IP of the target device (-i <IP>)" << std::endl;
		return ERR_PARAMETER_MISSING;
	}

	boost::system::error_code err;
	boost::asio::io_service io;
	boost::asio::ip::address_v6 target;

	target = hexabus::resolve(io, vm["ip"].as<std::string>(), err);
	if (err) {
		std::cerr << vm["ip"].as<std::string>() << " is not a valid IP address: " << err.message() << std::endl;
		return ERR_PARAMETER_FORMAT;
	}

	std::vector<std::array<uint8_t, UploadChunkSize> > chunks;

	if (vm.count("program")) {
		std::ifstream in(vm["program"].as<std::string>().c_str(),
				std::ios_base::in | std::ios::ate | std::ios::binary);
		if (!in) {
			std::cerr << "Error: Could not open input file: "
				<< vm["program"].as<std::string>() << std::endl;
			return ERR_PARAMETER_VALUE_INVALID;
		}
		in.unsetf(std::ios::skipws); // No white space skipping!

		size_t size = in.tellg();
		in.seekg(0, std::ios::beg);

		chunks.push_back({});
		chunks.back().fill(0);

		while (in && !in.eof()) {
			std::array<char, UploadChunkSize> chunk;
			chunk.fill(0);

			in.read(chunk.data(), chunk.size());
			if (in || in.eof()) {
				chunks.push_back({});
				memcpy(&chunks.back()[0], chunk.data(), chunk.size());
			} else {
				std::cerr << "Can't read program" << std::endl;
				return ERR_READ_FAILED;
			}
		}
	}

	if (vm.count("program")) {
		std::cout << "Uploading program, size=" << chunks.size() * UploadChunkSize << std::endl;
	} else {
		std::cout << "Clearing state machine" << std::endl;
	}

	try {
		hexabus::Socket socket(io, vm.count("retry") ? vm["retry"].as<int>() : 5);
		uint8_t chunkId = 0;

		ChunkSender sender(socket, target);
		RemoteStateMachine sm(socket, target);
		ErrorCode err;

		if ((err = sm.stop())) {
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
		if (vm.count("clear")) {
			std::array<uint8_t, UploadChunkSize> chunk;
			chunk.fill('\xff');

			for (chunkId = 1; chunkId < PROG_DEFAULT_LENGTH / UploadChunkSize; chunkId++) {
				err = sender.sendChunk(chunkId, chunk);
				if (err) {
					break;
				}
			}
		} else {
			for (chunkId = 0; chunkId < chunks.size(); chunkId++) {
				err = sender.sendChunk(chunkId, chunks[chunkId]);

				if (err) {
					break;
				}
			}
		}

		if (err) {
			std::cerr << "Failed to upload program" << std::endl;
			return err;
		}

		if ((err = sm.start())) {
			return err;
		}
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Network error: " << e.code().message() << std::endl;
		return ERR_NETWORK;
	}

	std::cout << "Program uploaded successfully." << std::endl;

	return ERR_NONE;
}

