#include <iostream>
#include "RtMidi/RtMidi.h"
#include <cstdlib>
#include <chrono>
#include <libhexanode/midi/midi_controller.hpp>
#include <libhexanode/midi/event_parser.hpp>
#include <libhexanode/common.hpp>
#include <libhexabus/common.hpp>
#include <libhexabus/socket.hpp>
#include <libhexanode/error.hpp>
// commandline parsing.
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/format.hpp>
namespace po = boost::program_options;

namespace {

class HexaDial {
private:
	hexabus::Socket& _socket;
	std::map<uint8_t, uint8_t> _dialValues, _lastSentValues;
	std::chrono::steady_clock::time_point _lastSendTime;

public:
	HexaDial(hexabus::Socket& socket)
		: _socket(socket)
	{
	}

	void operator()(uint8_t dial, uint8_t value)
	{
		using namespace std::chrono;

		_dialValues[dial] = value;
		if (_dialValues != _lastSentValues &&
			duration_cast<milliseconds>(steady_clock::now() - _lastSendTime).count() >= 250) {
			for (auto value : _dialValues) {
				if (_lastSentValues[value.first] != value.second) {
					uint32_t epid = 33 + value.first - 1;
					if (epid > 40) {
						std::cout << "EID out of range: " << epid << std::endl;
					} else {
						_socket.ioService().post([=] () {
							_socket.send(hexabus::InfoPacket<uint8_t>(epid, value.second));
						});
					}
				}
			}

			_lastSentValues = _dialValues;
			_lastSendTime = steady_clock::now();
		}
	}
};

class HexabusMidiDevice {
private:
	hexabus::Listener& _listener;
	hexabus::Socket& _socket;

	void onPacket(const hexabus::Packet& packet, const boost::asio::ip::udp::endpoint& from)
	{
		using namespace hexabus;

		if (auto* pq = dynamic_cast<const QueryPacket*>(&packet)) {
			if (pq->eid() == 0)
				_socket.send(InfoPacket<uint32_t>(pq->eid(), (1<<0) | (1<<25)), from);
			else if (pq->eid() == 32)
				_socket.send(InfoPacket<uint32_t>(pq->eid(), (1<<0) | (0xff << (33 - 32))), from);
			else if (pq->eid() % 32 == 0)
				_socket.send(InfoPacket<uint32_t>(pq->eid(), 1), from);
			else
				_socket.send(ErrorPacket(HXB_ERR_UNKNOWNEID), from);
		} else if (auto* epq = dynamic_cast<const EndpointQueryPacket*>(&packet)) {
			if (epq->eid() == 0)
				_socket.send(EndpointInfoPacket(epq->eid(), HXB_DTYPE_UINT32, "Hexabus MIDI device"), from);
			else if (epq->eid() == 25)
				_socket.send(EndpointInfoPacket(epq->eid(), HXB_DTYPE_UINT8, "Hexapush clicked button"), from);
			else if (33 <= epq->eid() && epq->eid() <= 40)
				_socket.send(
					EndpointInfoPacket(
						epq->eid(), HXB_DTYPE_UINT8,
						str(boost::format("Generic dial #%1%") % (epq->eid() - 32))), from);
			else
				_socket.send(ErrorPacket(HXB_ERR_UNKNOWNEID), from);
		}
	}

public:
	HexabusMidiDevice(hexabus::Listener& listener, hexabus::Socket& socket)
		: _listener(listener), _socket(socket)
	{
		namespace _ = std::placeholders;
		_socket.onPacketReceived(std::bind(&HexabusMidiDevice::onPacket, this, _::_1, _::_2));
		_listener.onPacketReceived(std::bind(&HexabusMidiDevice::onPacket, this, _::_1, _::_2));
	}
};

}


int main(int argc, char const* argv[])
{
  po::options_description desc("Usage: midi2hexabus --ip <ip> --interface <iface> --port <port> [additional options]");
  desc.add_options()
	("help,h", "print help message and quit")
	("verbose,v", "print verbose messages")
	("version", "print version and quit")
	("port,p", po::value<uint16_t>(), "port to use")
	("ip,i", po::value<std::string>(), "ip address to bind to")
	("interface,I", po::value<std::string>(), "interface to bind to");

	po::positional_options_description p;
	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
		po::notify(vm);
	} catch (const std::exception& e) {
		std::cerr << e.what() << '\n';
		return 1;
	}

	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return 1;
	}
	if (vm.count("version")) {
		hexanode::VersionInfo vi;
		std::cout << "hexanode version " << vi.get_version() << std::endl;
		return 0;
	}
	if (!vm.count("ip")) {
		std::cerr << "missing option --ip\n";
		return 1;
	}
	if (!vm.count("interface")) {
		std::cerr << "missing option --interface\n";
		return 1;
	}

	boost::system::error_code err;

	auto bindTo = boost::asio::ip::address_v6::from_string(vm["ip"].as<std::string>(), err);
	if (err) {
		std::cerr << "invalid IP address " << vm["ip"].as<std::string>() << '\n';
		return 1;
	}

	auto iface = vm["interface"].as<std::string>();

	hexanode::MidiController midi;
	hexanode::MidiEventParser parser;

	boost::asio::io_service io;

	try {
		hexabus::Socket socket(io);
		socket.bind({bindTo, 61616});
		socket.mcast_from(iface);

		hexabus::Listener listener(io);
		listener.listen(iface);

		HexabusMidiDevice device(listener, socket);
		HexaDial hexadial(socket);

		parser.onButtonEvent([&] (uint8_t button) {
				socket.send(hexabus::InfoPacket<uint8_t>(25, button));
				});
		parser.onDialEvent(hexadial);

		midi.open();

		auto nPorts = midi.num_ports();
		std::cout << "There are " << nPorts << " MIDI input sources available." << std::endl;

		uint16_t port = 0;
		if (vm.count("port"))
			port = vm["port"].as<uint16_t>() - 1;

		if (!nPorts) {
			std::cout << "Sorry, cannot continue without MIDI input devices." << std::endl;
			return 1;
		}

		midi.print_ports();
		midi.do_on_midievent(port, std::ref(parser));

		if (vm.count("verbose")) {
			std::cout << "Printing midi event debug messages." << std::endl;
			midi.do_on_midievent(port, [&] (const std::vector<unsigned char>& msg) {
				std::cout << "MIDI message: ";
				for (unsigned i = 0; i < msg.size(); i++)
					std::cout << (i ? ", " : "") << unsigned(msg[i]);
				std::cout << std::endl;
			});
		}

		midi.run();
		io.run();
	} catch (const hexanode::MidiException& ex) {
		std::cerr << "MIDI error: " << ex.what() << std::endl;
	} catch (const hexabus::NetworkException& ex) {
		std::cerr << "network error: " << ex.what() << ": " << ex.code().message() << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Unknown error: " << e.what() << std::endl;
	}

	return 0;
}

