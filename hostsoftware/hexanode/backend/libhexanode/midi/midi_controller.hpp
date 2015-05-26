#ifndef LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP
#define LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP 1

#include <libhexanode/common.hpp>
#include <boost/signals2.hpp>
#include <memory>
#include <atomic>
#include <thread>
#include "RtMidi/RtMidi.h"

namespace hexanode {

class MidiController {
public:
	typedef boost::signals2::signal<void (const std::vector<unsigned char>&)> on_midievent_t;
	typedef on_midievent_t::slot_type on_midievent_slot_t;

	MidiController();
	~MidiController();

	void open();
	boost::signals2::connection do_on_midievent(uint16_t port, const on_midievent_slot_t& slot);
	void run();
	void shutdown();

	uint16_t num_ports();
	void print_ports();

private:
	MidiController(const MidiController& original) = delete;
	MidiController& operator=(const MidiController& rhs) = delete;
	void midievent_loop();

	std::thread _t;
	std::atomic<bool> _terminate_midievent_loop;
	std::unique_ptr<RtMidiIn> _midiin;
	on_midievent_t _on_midievent;
};

};

#endif /* LIBHEXANODE_MIDI_MIDI_CONTROLLER_HPP */
