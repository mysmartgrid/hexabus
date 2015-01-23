#include "midi_controller.hpp"

#include "config.h"
#include <libhexanode/error.hpp>

using namespace hexanode;

#if HAS_LINUX == 1
# define RTMIDI_API RtMidi::LINUX_ALSA
#endif

MidiController::MidiController()
	: _terminate_midievent_loop(false)
{
}

MidiController::~MidiController()
{
	if (_t.joinable())
		shutdown();
}

void MidiController::open()
{
	try {
		_midiin.reset(new RtMidiIn(RTMIDI_API));
	} catch (const RtError& error) {
		throw MidiException(error.getMessage());
	}
}

boost::signals2::connection MidiController::do_on_midievent(uint16_t port, const on_midievent_slot_t& slot)
{
	_midiin->openPort(port);
	// Don't ignore sysex, timing, or active sensing messages.
	_midiin->ignoreTypes(false, false, false);

	return _on_midievent.connect(slot);
}

void MidiController::run()
{
	_t = std::thread([&] () {
		midievent_loop();
	});
}

void MidiController::shutdown()
{
	_terminate_midievent_loop = true;
	_t.join();
}

void MidiController::midievent_loop()
{
	std::vector<unsigned char> message;
	int nBytes;

	while (!_terminate_midievent_loop) {
		_midiin->getMessage(&message);

		if (message.size())
			_on_midievent(message);

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

uint16_t MidiController::num_ports()
{
	try {
		return _midiin->getPortCount();
	} catch (const RtError& error) {
		throw MidiException(error.getMessage());
	}
}

void MidiController::print_ports()
{
	for (unsigned i = 0; i < num_ports(); i++) {
		auto portName = _midiin->getPortName(i);
		std::cout << "  Input Port #" << i + 1 << ": " << portName << std::endl;
	}
}
