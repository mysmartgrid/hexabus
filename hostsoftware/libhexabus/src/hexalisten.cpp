#include <iostream>
#include <iomanip>
#include <libhexabus/socket.hpp>
#include <boost/program_options.hpp>

#include "shared.hpp"

using namespace hexabus;
namespace po = boost::program_options;

int main(int argc, char* argv[])
{
	bool oneline = false;
	std::vector<std::string> interfaces;

	po::options_description visibleOpts;
	visibleOpts.add_options()
		("help,h", "display this message")
		("oneline,o", po::bool_switch(&oneline), "one packet per line");

	po::options_description invisibleOpts;
	invisibleOpts.add_options()
		(",I", po::value<std::vector<std::string>>(&interfaces));

	po::options_description all;
	all.add(visibleOpts).add(invisibleOpts);

	po::positional_options_description positional;
	positional.add("-I", -1);

	po::variables_map vm;

	try {
		po::store(
			po::command_line_parser(argc, argv)
				.options(all)
				.positional(positional)
				.run(),
			vm);
		po::notify(vm);
	} catch (const std::exception& e) {
		std::cerr << "error in command line: unexpected option " << e.what() << std::endl;
		return 1;
	}

	if (vm.count("help") || interfaces.empty()) {
		std::cout << "Usage: hexalisten [options] <interface> [...]" << std::endl;
		std::cout << visibleOpts;
		return 0;
	}

	boost::asio::io_service io;
	hexabus::Listener listener(io);
	PacketPrinter printer(oneline);

	for (const auto& iface : interfaces) {
		try {
			listener.listen(iface);
		} catch (const hexabus::NetworkException& e) {
			std::cerr << "cannot listen on " << iface << ": " << e.code().message() << std::endl;
			return 1;
		}
	}

	listener.onPacketReceived(std::ref(printer));
	listener.onAsyncError([] (hexabus::GenericException e) {
		throw e;
	});

	try {
		io.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
