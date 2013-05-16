#include <iostream>
#include "RtMidi/RtMidi.h"
#include <cstdlib>
#include <libhexanode/midi/midi_controller.hpp>
#include <libhexanode/callbacks/print_callback.hpp>
#include <libhexanode/callbacks/button_callback.hpp>
#include <libhexanode/hexabus/hexapush.hpp>
#include <libhexanode/hexabus/hexadial.hpp>
#include <libhexanode/common.hpp>
#include <libhexabus/common.hpp>
#include <libhexanode/error.hpp>
// commandline parsing.
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>
namespace po = boost::program_options;


int main (int argc, char const* argv[]) {
  std::ostringstream oss;
  oss << "Usage: " << argv[0];
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "print help message and quit")
    ("verbose,v", "print verbose messages")
    ("version", "print version and quit")
    ("port,p", po::value<uint16_t>(), "port to use")
    ;
  po::positional_options_description p;

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).
      options(desc).positional(p).run(), vm);
  po::notify(vm);

  // Begin processing of commandline parameters.
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }
  if (vm.count("version")) {
    hexanode::VersionInfo::Ptr hxnode_version(new hexanode::VersionInfo());
    std::cout << "hexanode version " << hxnode_version->get_version() << std::endl;
    return 0;
  }

  hexanode::MidiController::Ptr midi_ctrl(
      new hexanode::MidiController());
  hexanode::ButtonCallback::Ptr btn_callback(
      new hexanode::ButtonCallback());
  hexanode::Callback::Ptr ptr_callback(
      new hexanode::PrintCallback());

  boost::asio::io_service io;
  hexabus::Socket socket(io);
  hexanode::HexaPush hexapush(socket);
  hexanode::HexaDial hexadial(socket);

  // Link the midi button event to a hexapush event.
  btn_callback->do_on_buttonevent(boost::bind(
            &hexanode::HexaPush::on_event,
            &hexapush, _1));
  btn_callback->do_on_dialevent(boost::bind(
            &hexanode::HexaDial::on_event,
            &hexadial, _1, _2));
  try {
    midi_ctrl->open();

    // Check inputs.
    uint16_t nPorts = midi_ctrl->num_ports();
    std::cout << "There are " << nPorts 
      << " MIDI input sources available." << std::endl;
    uint16_t port = 0;
    if (vm.count("port")) {
      port = vm["port"].as<uint16_t>() - 1;
    }
    if (nPorts == 0) {
      std::cout << "Sorry, cannot continue without MIDI input devices." << std::endl;
      exit(-1);
    } else {
      midi_ctrl->print_ports();
      midi_ctrl->do_on_midievent(port, boost::bind(
            &hexanode::Callback::on_event,
            btn_callback, _1));
      if (vm.count("verbose")) {
        std::cout << "Printing midi event debug messages." << std::endl;
        midi_ctrl->do_on_midievent(port, boost::bind(
              &hexanode::Callback::on_event,
              ptr_callback, _1));
      }
      midi_ctrl->run();
      while(true) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(250));
        hexadial.send_values();
      }
    }
  } catch (const hexanode::MidiException& ex) {
    std::cerr << "MIDI error: " << ex.what() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << "Unknown error: " << e.what() << std::endl;
  }

  midi_ctrl->shutdown();
  return 0;
}

