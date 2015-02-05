#include "libhexanode/midi/event_parser.hpp"

#include "libhexanode/midi/midi_codes.hpp"

#include <iostream>

namespace hexanode {

void MidiEventParser::operator()(const std::vector<unsigned char>& message)
{
	if (message.size() == 3) {
		switch (message[0]) {
		case PAD_PRESSED:
			_onButtonEvent(message[1]);
			break;

		case KNOB_TURNED:
			// The MIDI standard says that values are from 0..127. Let's 
			// scale to full uint8_t range.
			_onDialEvent(message[1], 2 * message[2]);
			break;
		}

		return;
	}

	std::cout << "received event of unknown type - ignoring." << std::endl;
}

boost::signals2::connection MidiEventParser::onButtonEvent(const button_event_t::slot_type& handler)
{
	return _onButtonEvent.connect(handler);
}

boost::signals2::connection MidiEventParser::onDialEvent(const dial_event_t::slot_type& handler)
{
	return _onDialEvent.connect(handler);
}

}
