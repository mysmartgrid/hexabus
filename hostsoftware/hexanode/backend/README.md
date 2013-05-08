MIDI notes:

You must start JACK before using hexanode like this:

    $ jackd -r -d alsa -Xraw

The -r prevents jackd to go into realtime mode (not needed). It uses the
ALSA backend and transfers control of the MIDI devices to the jack
library itself.

The LPD8 device should be recognized by the jack library:
  
    scan: added port hw:1,0,0 in-hw-1-0-0-LPD8-MIDI-1
    scan: added port hw:1,0,0 out-hw-1-0-0-LPD8-MIDI-1

See also:
http://murks.lima-city.de/serendipity/index.php?/archives/7-ALSA-and-JACK-MIDI-explained-by-a-dummy-for-dummies.html


