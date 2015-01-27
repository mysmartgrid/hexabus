#ifndef LIBHEXANODE_MIDI_EVENT__PARSER_HPP_6C2C588A492325A5
#define LIBHEXANODE_MIDI_EVENT__PARSER_HPP_6C2C588A492325A5

#include <vector>
#include <boost/signals2.hpp>

namespace hexanode {

class MidiEventParser {
public:
	typedef boost::signals2::signal<void (uint8_t)> button_event_t;
	typedef boost::signals2::signal<void (uint8_t, uint8_t)> dial_event_t;

private:
	button_event_t _onButtonEvent;
	dial_event_t _onDialEvent;

public:
	void operator()(const std::vector<unsigned char>& message);

	boost::signals2::connection onButtonEvent(const button_event_t::slot_type& handler);
	boost::signals2::connection onDialEvent(const dial_event_t::slot_type& handler);
};

}

#endif
