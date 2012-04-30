hexaswitch
==========

How to install
--------------

You need:
1. boost 1.48 or newer

   Note: If you install boost from the sources on Ubuntu, make sure that libbz2-dev is installed beforehand. (b2 should compile everything and not complain that anything was not updated)

2. libklio

   You can grab it from https://github.com/mysmartgrid/libklio

Now just run make in this folder (hexaswitch). It should drop two executable into build/src: hexaswitch and hexalog. If you want to install system wide, you can run (sudo) make install, too.

How to use
----------

### Listening to hexabus broadcasts

Just run `hexaswitch listen`. It dumps all the broadcasts to the console.

### Controlling devices (sending WRITE, QUERY or EPQUERY packets)

Run `hexaswitch <hostname> <command>`, substituting <hostname> for your device's IP address. Commands are:

* `get <EID>` -- sends a QUERY for the endpoint with the ID <EID> and waits for a reply, which is printed on the console
* `set <EID> <datatype> <value>` -- sends a WRITE to the endpoint with the ID <EID>. <value> is the value you want to write. For datatypes, see the output of hexaswitch when called without arguments.
* `epquery <EID>` -- sends an EPQUERY, waits for the response and prints it to the console.

There are several shorthand commands. Just run hexaswitch without any arguments to see a list.
