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
#include <algorithm>
#include <vector>
#include <typeinfo>
namespace po = boost::program_options;

#include "../../../shared/hexabus_packet.h"
#include "../../../shared/hexabus_definitions.h"
#include "../../../shared/endpoints.h"

#pragma GCC diagnostic warning "-Wstrict-aliasing"

po::variable_value get_mandatory_parameter(
    po::variables_map vm,
    std::string param_id,
    std::string error_message
    )
{
  po::variable_value retval;
  try {
    if (! vm.count(param_id)) {
      std::cerr << error_message << std::endl;
      exit(-1);
    } else {
      retval=(vm[param_id]);
    }
  } catch (std::exception& e) {
    std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
    exit(-1);
  }
  return retval;
}

void assert_statemachine_state(hexabus::Socket* network, const std::string& ip_addr, STM_state_t req_state) {
	boost::asio::ip::address_v6 dest = boost::asio::ip::address_v6::from_string(ip_addr);

	network->send(hexabus::WritePacket<uint8_t>(EP_SM_CONTROL, req_state), dest);
	network->send(hexabus::QueryPacket(EP_SM_CONTROL), dest);

	while (true) {
		std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> pair;
		try {
			pair = network->receive();
		} catch (const hexabus::GenericException& e) {
			const hexabus::NetworkException* nerror;
			if ((nerror = dynamic_cast<const hexabus::NetworkException*>(&e))) {
				std::cerr << "Error receiving packet: " << nerror->code().message() << std::endl;
			} else {
				std::cerr << "Error receiving packet: " << e.what() << std::endl;
			}
			exit(1);
		}

		if (pair.second.address() == dest) {
			const hexabus::InfoPacket<uint8_t> *u8 = dynamic_cast<const hexabus::InfoPacket<uint8_t>*>(pair.first.get());
			const hexabus::TypedPacket *t = dynamic_cast<const hexabus::TypedPacket*>(pair.first.get());
			if (u8) {
				switch (req_state) {
					case STM_STATE_STOPPED:
						if (u8->value() == STM_STATE_STOPPED) {
							std::cout << "State machine has been stopped successfully" << std::endl;
						} else {
							std::cerr << "Failed to stop state machine - aborting." << std::endl;
							exit(-2);
						}
						break;

					case STM_STATE_RUNNING:
						if (u8->value() == STM_STATE_RUNNING) {
							std::cout << "State machine is running." << std::endl;
						} else {
							std::cerr << "Failed to start state machine - aborting." << std::endl;
							exit(-2);
						}
						break;

					default:
						std::cout << "Unexpected STM_STATE requested - aborting." << std::endl;
						exit(-3);
						break;
				}
			} else if (t && t->eid() == EP_SM_CONTROL) {
				std::cout << "Expected uint8 data in packet - got something different." << std::endl;
				exit(-4);
			}
			break;
		}
	}
}

bool send_chunk(hexabus::Socket* network, const std::string& ip_addr, uint8_t chunk_id, const std::vector<char>& chunk) {
	//std::cout << "Sending chunk " << (int) chunk_id << std::endl;
	std::vector<char> bin_data;
	bin_data.push_back(chunk_id); 
	bin_data.insert(bin_data.end(), chunk.begin(), chunk.end());

	network->send(hexabus::WritePacket<std::vector<char> >(EP_SM_UP_RECEIVER, bin_data), boost::asio::ip::address_v6::from_string(ip_addr));

	while (true) {
		std::pair<hexabus::Packet::Ptr, boost::asio::ip::udp::endpoint> p =
			network->receive();

		if (p.second.address() == boost::asio::ip::address::from_string(ip_addr)) {
			const hexabus::InfoPacket<bool> *i = dynamic_cast<const hexabus::InfoPacket<bool>*>(p.first.get());
			if (i) {
				return i->value();
			} else {
				std::cout << "?";
			}
		}
	}
}

int main(int argc, char** argv) {

  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " IP [additional options] ACTION";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce help message")
    ("version", "print libhexabus version and exit")
    ("ip,i", po::value<std::string>(), "the hostname to connect to. If this option is not set, the target IP address from the program file will be used.")
    ("interface,I", po::value<std::string>(), "the interface to use for outgoing messages")
    ("program,p", po::value<std::string>(), "the state machine program to be uploaded")
    ;
  po::positional_options_description p;
  p.add("ip", 1);
  p.add("program", 1);
  po::variables_map vm;

  // Begin processing of commandline parameters.
  try {
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
    exit(-1);
  }

  std::string command;
  std::vector<char> program;

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  if (vm.count("version")) {
    std::cout << "hexaswitch -- command line hexabus client" << std::endl;
    std::cout << "libhexabus version " << hexabus::version() << std::endl;
    return 0;
  }

  if (!vm.count("program")) {
    std::cout << "Cannot proceed without a program (-p <FILE>)" << std::endl;
    exit(-1);
  } else {
    std::ifstream in(vm["program"].as<std::string>().c_str(),
        std::ios_base::in | std::ios::ate | std::ios::binary);
    if (!in) {
      std::cerr << "Error: Could not open input file: "
        << vm["program"].as<std::string>() << std::endl;
      exit(-1);
    }
    in.unsetf(std::ios::skipws); // No white space skipping!

    size_t size = in.tellg();
    in.seekg(0, std::ios::beg);

    // first, read target IP
    const size_t IP_ADDR_LENGTH = 16;
    unsigned char* ip_buffer = new unsigned char[IP_ADDR_LENGTH];
    in.read((char*)ip_buffer, IP_ADDR_LENGTH);

    // then read program into buffer
    char* buffer = new char[size - IP_ADDR_LENGTH];
    in.read(buffer, size - IP_ADDR_LENGTH);
    //std::string instr(buffer, size);
    program.insert(program.end(), buffer, buffer+(size - IP_ADDR_LENGTH));
    delete buffer;
    in.close();

	boost::asio::io_service io;
  hexabus::Socket* network;

  if (vm.count("interface")) {
    std::string interface=(vm["interface"].as<std::string>());
    std::cout << "Using interface " << interface << std::endl;
    network=new hexabus::Socket(io, interface);
  } else {
    network=new hexabus::Socket(io);
  }

  std::string ip_addr;
  if(vm.count("ip")) { // if we have an IP address given at the command line, use it
    ip_addr = vm["ip"].as<std::string>();
  } else { // if there is no IP address at the command line, read the one from the input file
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for(size_t i = 0; i < IP_ADDR_LENGTH; ++i) {
      if(i && !(i % 2)) // output a : after every two bytes
        oss << ":";
      oss << std::setw(2) << (unsigned int)ip_buffer[i];
    }
    std::cout << std::endl << "Using target IP from program file: " << oss.str() << std::endl;
    ip_addr = oss.str();
  }

  delete ip_buffer;

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
  assert_statemachine_state(network, ip_addr, STM_STATE_STOPPED);

  uint8_t chunk_id = 0;
  uint8_t MAX_TRY = 3;
  std::cout << "Uploading program, size=" << program.size() << std::endl;
  while((64 * chunk_id) < program.size()) {
    uint8_t failure_counter=0;
    std::vector<char> to_send(program.begin() + (64*chunk_id), program.begin() + std::min(64 * (chunk_id + 1), (int) program.size()));

    bool sent = false;
    while(!sent) {
      if(!send_chunk(network, ip_addr, chunk_id, to_send)) {
        // TODO: Check if the ACK/NAK refers to the current chunk id
        std::cout << "F";
        failure_counter++; // if sending fails, increase failure counter
        if (failure_counter >= MAX_TRY) {
          std::cout << "Maximum number of retransmissions exceeded." << std::endl;
          delete network;
          exit(1);
        }
      } else {
        std::cout << ".";
        sent = true;
      }
      std::cout << std::flush;
    } 
    chunk_id++;
  }

  std::cout << std::endl;
  std::cout << "Upload completed." << std::endl;
  assert_statemachine_state(network, ip_addr, STM_STATE_RUNNING);
  std::cout << "Program uploaded successfully." << std::endl;

  delete network;
  }
  exit(0);
}

