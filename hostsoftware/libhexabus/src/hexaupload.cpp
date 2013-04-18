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

#include "../../../shared/hexabus_definitions.h"
#include "../../../shared/hexabus_statemachine_structs.h"
#include "../../../shared/endpoints.h"
#include "resolv.hpp"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

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
	ERR_PARAMETER_INVALID = 9,

	ERR_OTHER = 127
};

class RetryingPacketSender {
	protected:
		boost::asio::deadline_timer timeout;
		hexabus::Socket& socket;
		boost::asio::ip::address_v6 target;
		int retryLimit;
		boost::signals2::scoped_connection errorHandler;

		const hexabus::Packet* packet;
		ErrorCode result;

		int failCount;

		void armTimer()
		{
			timeout.expires_from_now(boost::posix_time::seconds(1));
			timeout.async_wait(boost::bind(&RetryingPacketSender::onTimeout, this, _1));
		}

		void failOne(char sign)
		{
			failCount++;
			if (failCount > retryLimit || retryLimit < 0) {
				finish(ERR_MAXRETRY_EXCEEDED);
			} else {
				std::cout << sign << std::flush;
				sendPacket();
			}
		}

		void onTimeout(const boost::system::error_code& err)
		{
			if (!err) {
				failOne('T');
				armTimer();
			}
		}

		void onError(const hexabus::GenericException& e)
		{
			const hexabus::NetworkException* ne = dynamic_cast<const hexabus::NetworkException*>(&e);
			if (ne) {
				std::cerr << "Error receiving packet: " << ne->code().message() << std::endl;
				finish(ERR_NETWORK);
			} else {
				std::cerr << "Error receiving packet: " << e.what() << std::endl;
				finish(ERR_NETWORK);
			}
		}

		virtual void sendPacket()
		{
			socket.send(*packet, target);
		}

		void finish(ErrorCode result)
		{
			this->result = result;
			socket.ioService().stop();
		}

	public:
		RetryingPacketSender(hexabus::Socket& socket, const boost::asio::ip::address_v6& target, int retryLimit)
			: timeout(socket.ioService()), socket(socket), target(target), retryLimit(retryLimit),
				errorHandler(socket.onAsyncError(boost::bind(&RetryingPacketSender::onError, this, _1)))
		{
		}

		ErrorCode send(const hexabus::Packet& packet)
		{
			this->packet = &packet;

			sendPacket();

			armTimer();
			failCount = 0;

			socket.ioService().reset();
			socket.ioService().run();

			timeout.cancel();

			return result;
		}
};

class RemoteStateMachine : protected RetryingPacketSender {
	private:
		boost::signals2::connection replyHandler;

		STM_state_t reqState;

		void onReply(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& from)
		{
			const hexabus::InfoPacket<uint8_t>& u8 = dynamic_cast<const hexabus::InfoPacket<uint8_t>&>(packet);

			switch (reqState) {
				case STM_STATE_STOPPED:
					if (u8.value() == STM_STATE_STOPPED) {
						std::cout << "State machine stopped" << std::endl;
						finish(ERR_NONE);
					} else {
						finish(ERR_SM_OP_FAILED);
					}
					break;

				case STM_STATE_RUNNING:
					if (u8.value() == STM_STATE_RUNNING) {
						std::cout << "State machine is running." << std::endl;
						finish(ERR_NONE);
					} else {
						finish(ERR_SM_OP_FAILED);
					}
					break;
			}
		}

		virtual void sendPacket()
		{
			RetryingPacketSender::sendPacket();
			socket.send(hexabus::QueryPacket(EP_SM_CONTROL), target);
		}

		ErrorCode setAndCheckState(STM_state_t state)
		{
			reqState = state;
			ErrorCode err = send(hexabus::WritePacket<uint8_t>(EP_SM_CONTROL, state));

			if (err) {
				std::cout << std::endl
					<< "Failed to " << (state == STM_STATE_RUNNING ? "start" : "stop") << " state machine - aborting." << std::endl;
			}
			return err;
		}

	public:
		RemoteStateMachine(hexabus::Socket& socket, const boost::asio::ip::address_v6& target, int retryLimit)
			: RetryingPacketSender(socket, target, retryLimit)
		{
			namespace hf = hexabus::filtering;

			replyHandler = socket.onPacketReceived(
					boost::bind(&RemoteStateMachine::onReply, this, _1, _2),
					hf::sourceIP() == target && hf::isInfo<uint8_t>() && hf::eid() == EP_SM_CONTROL);
		}

		~RemoteStateMachine()
		{
			replyHandler.disconnect();
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

static const size_t UploadChunkSize = EE_STATEMACHINE_CHUNK_SIZE;

class ChunkSender : protected RetryingPacketSender {
	private:
		boost::signals2::connection replyHandler;

		void onReply(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& from)
		{
			if (dynamic_cast<const hexabus::InfoPacket<bool>&>(packet).value()) {
				std::cout << "." << std::flush;
				finish(ERR_NONE);
			} else {
				failOne('F');
			}
		}

	public:
		ChunkSender(hexabus::Socket& socket, const boost::asio::ip::address_v6& target, int retryLimit)
			: RetryingPacketSender(socket, target, retryLimit)
		{
			namespace hf = hexabus::filtering;

			replyHandler = socket.onPacketReceived(
					boost::bind(&ChunkSender::onReply, this, _1, _2),
					hf::sourceIP() == target && hf::isInfo<bool>() && hf::eid() == EP_SM_UP_ACKNAK);
		}

		~ChunkSender()
		{
			replyHandler.disconnect();
		}

		ErrorCode sendChunk(uint8_t chunkId, const boost::array<char, UploadChunkSize>& chunk)
		{
			boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> packet_data;

			packet_data[0] = chunkId;
			std::copy(chunk.begin(), chunk.end(), packet_data.begin() + 1);

			return send(hexabus::WritePacket<boost::array<char, HXB_66BYTES_PACKET_MAX_BUFFER_LENGTH> >(EP_SM_UP_RECEIVER, packet_data));
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

	if(vm.count("program") && vm.count("clear")) {
		std::cout << "--program and --clear are mutually exclusive." << std::endl;
		return ERR_PARAMETER_INVALID;
	}

	if (!vm.count("program") && !vm.count("clear")) {
		std::cout << "Cannot proceed without a program (-p <FILE>)" << std::endl;
		return ERR_PARAMETER_MISSING;
	}
	if (!vm.count("ip") && !vm.count("program")) {
		std::cout << "Cannot proceed without IP of the target device (-i <IP>)" << std::endl;
		return ERR_PARAMETER_MISSING;
	}

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

	boost::asio::ip::address_v6 target;

	// first, read target IP
	if (vm.count("program")) {
		boost::asio::ip::address_v6::bytes_type ipBuffer;
		in.read(reinterpret_cast<char*>(ipBuffer.c_array()), ipBuffer.size());
		target = boost::asio::ip::address_v6(ipBuffer);
	}

	boost::asio::io_service io;

	if (vm.count("ip")) {
		boost::system::error_code err;

		target = hexabus::resolve(io, vm["ip"].as<std::string>(), err);
		if (err) {
			std::cerr << vm["ip"].as<std::string>() << " is not a valid IP address: " << err.message() << std::endl;
			return ERR_PARAMETER_FORMAT;
		}
	} else if (!vm.count("program")) {
		std::cerr << "Target device IP not specified" << std::endl;
		return ERR_PARAMETER_MISSING;
	}

	if (vm.count("program")) {
		std::cout << "Uploading program, size=" << (size - in.tellg()) << std::endl;
	} else {
		// fill program memory with zeros.
		std::cout << "Clearing state machine" << std::endl;
	}

	try {
		hexabus::Socket socket(io);
		uint8_t chunkId = 0;
		int retryLimit = vm.count("retry") ? vm["retry"].as<int>() : 3;
		ChunkSender sender(socket, target, retryLimit);
		RemoteStateMachine sm(socket, target, retryLimit);
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
		boost::array<char, UploadChunkSize> chunk;
		chunk.assign(0);

		if (vm.count("clear")) {
			for (chunkId = 0; chunkId < PROG_DEFAULT_LENGTH / EE_STATEMACHINE_CHUNK_SIZE; chunkId++) {
				err = sender.sendChunk(chunkId, chunk);
				if (err) {
					break;
				}
			}
		} else {
			while (in && !in.eof()) {
				in.read(chunk.c_array(), chunk.size());
				if (in) {
					err = sender.sendChunk(chunkId, chunk);

					if (err) {
						break;
					}

					chunkId++;
				} else if (!in.eof()) {
					std::cerr << "Can't read program" << std::endl;
					return ERR_READ_FAILED;
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

