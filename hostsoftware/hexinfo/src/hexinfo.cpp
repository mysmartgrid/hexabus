#include <iostream>
#include <fstream>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <libhexabus/common.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/endpoint_registry.hpp>
#include <hbt/Lang/ast.hpp>
#include <hbt/Lang/astprinter.hpp>
#include <hbt/Lang/parser.hpp>

#include "../../../shared/hexabus_types.h"

namespace po = boost::program_options;

namespace {

struct DiscoveredEP {
	uint32_t eid;
	hexabus::hxb_datatype type;
	std::string name;

	bool operator<(const DiscoveredEP& other) const { return eid < other.eid; }

	const char* typeName() const
	{
		switch (type) {
		case hexabus::HXB_DTYPE_BOOL: return "Bool"; break;
		case hexabus::HXB_DTYPE_UINT8: return "UInt8"; break;
		case hexabus::HXB_DTYPE_UINT16: return "UInt16"; break;
		case hexabus::HXB_DTYPE_UINT32: return "UInt32"; break;
		case hexabus::HXB_DTYPE_UINT64: return "UInt64"; break;
		case hexabus::HXB_DTYPE_SINT8: return "Int8"; break;
		case hexabus::HXB_DTYPE_SINT16: return "Int16"; break;
		case hexabus::HXB_DTYPE_SINT32: return "Int32"; break;
		case hexabus::HXB_DTYPE_SINT64: return "Int64"; break;
		case hexabus::HXB_DTYPE_FLOAT: return "Float"; break;
		case hexabus::HXB_DTYPE_128STRING: return "String"; break;
		case hexabus::HXB_DTYPE_16BYTES: return "Binary (16 bytes)"; break;
		case hexabus::HXB_DTYPE_65BYTES: return "Binary (65 bytes)"; break;
		case hexabus::HXB_DTYPE_UNDEFINED: return "(undefined)"; break;
		}
		return "(unknown)";
	}
};

struct DiscoveredDev {
	boost::asio::ip::address_v6 address;
	std::string name;
	std::vector<DiscoveredEP> endpoints;

	bool operator<(const DiscoveredDev& other) const { return address < other.address; }
};

struct LogStream {
	bool ignore;
	std::ostream& into;

	template<typename T>
	LogStream& operator<<(const T& t)
	{
		if (!ignore)
			into << t;
		return *this;
	}

	LogStream& operator<<(std::ostream& (*func)(std::ostream&))
	{
		if (!ignore)
			into << func;
		return *this;
	}
};

struct DiscoverContext {
	hexabus::Socket& socket;
	const boost::posix_time::time_duration queryInterval;
	const unsigned retries;
	const bool verbose;

	DiscoverContext(hexabus::Socket& socket, boost::posix_time::time_duration interval, unsigned retries, bool verbose)
		: socket(socket), queryInterval(interval), retries(retries), verbose(verbose)
	{}

	LogStream vout()
	{
		return LogStream{!verbose, std::cout};
	}

	template<typename Fn>
	bool retryPacket(const hexabus::Packet& packet, boost::asio::ip::address_v6 target,
			Fn fn, const hexabus::Socket::filter_t& filter)
	{
		bool gotResponse = false;
		boost::asio::deadline_timer timer(socket.ioService());

		auto response = [&] (const hexabus::Packet& packet, boost::asio::ip::udp::endpoint ep) {
			if (fn(packet, ep)) {
				gotResponse = true;
				timer.cancel();
			}
		};

		boost::signals2::scoped_connection sc(socket.onPacketReceived(response, filter));

		for (auto tries = retries; !gotResponse && tries != 0; tries--) {
			timer.expires_from_now(queryInterval);
			timer.async_wait([&] (boost::system::error_code) {
				socket.ioService().stop();
			});

			vout() << '.' << std::flush;
			socket.send(packet, target);

			socket.ioService().reset();
			socket.ioService().run();
		}

		return gotResponse;
	}
};

struct NameSanitizer {
	bool forHumans;

	std::string sanitizeString(const std::string& name)
	{
		std::string result;

		for (auto c : name) {
			if (isalnum(c) && (!result.empty() || isalpha(c))) {
				result += c;
				continue;
			}

			result += '_';
			char buf[10];
			sprintf(buf, "%02x", c);
			result += buf;
		}

		return result;
	}

	std::string sanitizeName(const DiscoveredEP& ep)
	{
		if (forHumans)
			return sanitizeString(ep.name) + "_EP";

		return "ep_" + std::to_string(ep.eid);
	}

	std::string sanitizeName(const DiscoveredDev& dev)
	{
		if (forHumans)
			return sanitizeString(dev.name) + "_Dev";

		std::string result = "dev_" + dev.address.to_string();

		for (auto pos = result.find(':'); pos != result.npos; pos = result.find(':'))
			result[pos] = '_';

		return result;
	}
};

}

static std::set<boost::asio::ip::address_v6> enumerateDevices(DiscoverContext& dc)
{
	namespace hf = hexabus::filtering;

	std::set<boost::asio::ip::address_v6> addresses;

	auto response = [&] (const hexabus::Packet&, boost::asio::ip::udp::endpoint ep) {
		addresses.insert(ep.address().to_v6());
		return false;
	};

	dc.vout() << "Sending discovery packets ";
	dc.retryPacket(hexabus::QueryPacket(0), dc.socket.GroupAddress, response, hf::isInfo<uint32_t>() && (hf::eid() == 0UL));
	dc.vout() << std::endl;

	return addresses;
}

static void enumerateEndpoints(DiscoverContext& dc, DiscoveredDev& device)
{
	namespace hf = hexabus::filtering;

	std::set<uint32_t> eids;

	for (uint32_t epDesc = 0; epDesc < 256; epDesc += 32) {
		auto response = [&] (const hexabus::Packet& packet, boost::asio::ip::udp::endpoint ep) {
			uint32_t currentEP = epDesc + 1;
			uint32_t descValue = static_cast<const hexabus::InfoPacket<uint32_t>&>(packet).value() >> 1;

			while (descValue) {
				if (descValue & 1)
					eids.insert(currentEP);

				descValue >>= 1;
				currentEP++;
			}
			return true;
		};

		dc.vout() << "Querying descriptor " << epDesc << ' ';
		if (dc.retryPacket(hexabus::QueryPacket(epDesc), device.address, response, hf::isInfo<uint32_t>()))
			dc.vout() << std::endl;
		else
			dc.vout() << " no reply" << std::endl;
	}

	while (!eids.empty()) {
		uint32_t eid = *eids.begin();
		eids.erase(eid);

		auto response = [&] (const hexabus::Packet& packet, boost::asio::ip::udp::endpoint ep) {
			auto& info = static_cast<const hexabus::EndpointInfoPacket&>(packet);

			device.endpoints.push_back({ eid, hexabus::hxb_datatype(info.datatype()), info.value() });
			return true;
		};

		dc.vout() << "Querying endpoint " << eid << ' ';
		if (dc.retryPacket(hexabus::EndpointQueryPacket(eid), device.address, response, hf::isEndpointInfo()))
			dc.vout() << std::endl;
		else
			dc.vout() << " no reponse" << std::endl;
	}
}

static boost::optional<DiscoveredDev> queryDevice(DiscoverContext& dc, boost::asio::ip::address_v6 addr)
{
	namespace hf = hexabus::filtering;

	DiscoveredDev result;

	auto response = [&] (const hexabus::Packet& packet, boost::asio::ip::udp::endpoint ep) {
		result = {
			addr,
			static_cast<const hexabus::EndpointInfoPacket&>(packet).value(),
			{}
		};
		return true;
	};

	dc.vout() << "Querying device " << addr << std::endl
		<< "Sending EPQuery ";
	if (!dc.retryPacket(hexabus::EndpointQueryPacket(0), addr, response, hf::isEndpointInfo())) {
		dc.vout() << " no response" << std::endl;
		return {};
	}

	dc.vout() << std::endl;
	enumerateEndpoints(dc, result);

	return result;
}



static void printEndpoint(std::ostream& out, const DiscoveredEP& ep)
{
	out << "Endpoint information:" << std::endl
		<< "\tEndpoint ID:\t" << ep.eid << std::endl
		<< "\tName:       \t" << ep.name << std::endl
		<< "\tDatatype:   \t" << ep.typeName() << std::endl;
	out << std::endl;
}

static void printDevice(std::ostream& out, const DiscoveredDev& dev)
{
	out << "Device information:" << std::endl
		<< "\tIP address: \t" << dev.address << std::endl
		<< "\tDevice name:\t" << dev.name << std::endl
		<< "\tEndpoints:  \t";

	unsigned put = 0;
	for (auto& ep : dev.endpoints) {
		if (put++)
			out << ' ';
		out << ep.eid;
	}
	out << std::endl;
}

static bool writeDevJSON(std::ostream& out, const DiscoveredDev& dev, NameSanitizer san)
{
	auto json = [] (const std::string& s) {
		std::stringstream sbuf;
		for (char c : s) {
			switch (c) {
			case '"': sbuf << "\\\"";
			case '\n': sbuf << "\\\n";
			case '\t': sbuf << "\\\t";
			case '\r': sbuf << "\\\r";
			case '\v': sbuf << "\\\v";
			default: sbuf << c;
			}
		}
		return sbuf.str();
	};

	if (dev.name.empty())
		return false;

	out
		<< '{' << std::endl
		<< R"(	"name": ")" << json(dev.name) << R"(",)" << std::endl
		<< R"(	"sm_name": ")" << san.sanitizeName(dev) << R"(",)" << std::endl
		<< R"(	"ip": ")" << dev.address << R"(",)" << std::endl
		<< R"(	"endpoints": [)" << std::endl
		<< R"(		)";

	unsigned put = 0;
	for (auto& ep : dev.endpoints) {
		if (ep.eid % 32 == 0)
			continue;
		if (put++)
			out << ", ";

		out
			<< R"({)" << std::endl
			<< R"(			"eid": )" << ep.eid << ',' << std::endl
			<< R"(			"sm_name": ")" << san.sanitizeName(ep) << R"(",)" << std::endl
			<< R"(			"type": ")" << ep.typeName() << R"(",)" << std::endl;

		static hexabus::EndpointRegistry epr;
		auto epit = epr.find(ep.eid);
		if (epit != epr.end()) {
			out
				<< R"(			"unit": ")" << json(epit->second.unit().get_value_or("")) << R"(",)" << std::endl
				<< R"(			"description": ")" << json(epit->second.description()) << R"(",)" << std::endl
				<< R"(			"function": ")";
			switch (epit->second.function()) {
			case hexabus::EndpointDescriptor::sensor: out << "sensor"; break;
			case hexabus::EndpointDescriptor::actor: out << "actor"; break;
			case hexabus::EndpointDescriptor::infrastructure: out << "infrastructure"; break;
			}
			out << R"(")" << std::endl;
		}

		out << "		}";
	}

	out << ']' << std::endl << "	}";

	return true;
}

static void writeDevicesJSON(std::ostream& out, const std::vector<DiscoveredDev>& devices, NameSanitizer san)
{
	out << R"({"devices": [)";

	unsigned put = 0;
	for (auto& dev : devices) {
		if (put++)
			out << ", ";
		writeDevJSON(out, dev, san);
	}

	out << std::endl << "]}" << std::endl;
}

static std::ostream& openFile(const std::string& path, std::unique_ptr<std::ofstream>& ofPtr)
{
	if (path == "-")
		return std::cout;

	ofPtr.reset(new std::ofstream(path, std::ios::out));
	if (!*ofPtr) {
		std::cerr << "could not open file " << path << std::endl;
		exit(1);
	}

	return *ofPtr;
}

static bool fileExists(const std::string& path)
{
	std::fstream file(path);
	return !file.fail();
}

static void updateEPFile(const std::string& path, const std::vector<DiscoveredDev>& devices, NameSanitizer san)
{
	std::map<uint32_t, hbt::lang::Endpoint> epsInFile;
	std::string noFile;
	hbt::lang::SourceLocation noSloc(&noFile, 0, 0);

	if (path != "-" && fileExists(path)) {
		auto tu = hbt::lang::Parser({}).parse(path);
		for (auto& part : tu->items()) {
			if (auto ep = dynamic_cast<hbt::lang::Endpoint*>(part.get())) {
				epsInFile.emplace(ep->eid(), std::move(*ep));
			}
		}
	}

	for (auto& dev : devices) {
		for (auto& ep : dev.endpoints) {
			hbt::lang::Type type;
			switch (ep.type) {
			case hexabus::HXB_DTYPE_BOOL: type = hbt::lang::Type::Bool; break;
			case hexabus::HXB_DTYPE_UINT8: type = hbt::lang::Type::UInt8; break;
			case hexabus::HXB_DTYPE_UINT16: type = hbt::lang::Type::UInt16; break;
			case hexabus::HXB_DTYPE_UINT32: type = hbt::lang::Type::UInt32; break;
			case hexabus::HXB_DTYPE_UINT64: type = hbt::lang::Type::UInt64; break;
			case hexabus::HXB_DTYPE_SINT8: type = hbt::lang::Type::Int8; break;
			case hexabus::HXB_DTYPE_SINT16: type = hbt::lang::Type::Int16; break;
			case hexabus::HXB_DTYPE_SINT32: type = hbt::lang::Type::Int32; break;
			case hexabus::HXB_DTYPE_SINT64: type = hbt::lang::Type::Int64; break;
			case hexabus::HXB_DTYPE_FLOAT: type = hbt::lang::Type::Float; break;

			default:
				continue;
			}
			epsInFile.emplace(
				ep.eid,
				hbt::lang::Endpoint(
					noSloc,
					hbt::lang::Identifier(noSloc, san.sanitizeName(ep)),
					ep.eid,
					type,
					hbt::lang::EndpointAccess::Broadcast | hbt::lang::EndpointAccess::Write));
		}
	}

	std::vector<std::unique_ptr<hbt::lang::ProgramPart>> tuParts;
	for (auto& pair : epsInFile)
		tuParts.emplace_back(std::unique_ptr<hbt::lang::ProgramPart>(new auto(std::move(pair.second))));

	hbt::lang::TranslationUnit tu(
		std::unique_ptr<std::string>(new auto(path)),
		std::move(tuParts));

	std::unique_ptr<std::ofstream> ofPtr;
	hbt::lang::ASTPrinter(openFile(path, ofPtr)).visit(tu);
}

static void updateDevFile(const std::string& path, const std::vector<DiscoveredDev>& devices, NameSanitizer san)
{
	std::map<std::string, hbt::lang::Device> devsInFile;
	std::string noFile;
	hbt::lang::SourceLocation noSloc(&noFile, 0, 0);

	if (path != "-" && fileExists(path)) {
		auto tu = hbt::lang::Parser({}).parse(path);
		for (auto& part : tu->items()) {
			if (auto dev = dynamic_cast<hbt::lang::Device*>(part.get())) {
				devsInFile.emplace(dev->name().name(), std::move(*dev));
			}
		}
	}

	for (auto& dev : devices) {
		devsInFile.erase(dev.name);

		auto addrBytes = dev.address.to_bytes();
		std::array<uint8_t, 16> address;
		std::copy(addrBytes.begin(), addrBytes.end(), address.begin());

		std::vector<hbt::lang::Identifier> endpoints;
		for (auto& ep : dev.endpoints)
			endpoints.emplace_back(noSloc, san.sanitizeName(ep));

		devsInFile.emplace(
			dev.name,
			hbt::lang::Device(
				noSloc,
				hbt::lang::Identifier(noSloc, san.sanitizeName(dev)),
				address,
				std::move(endpoints)));
	}

	std::vector<std::unique_ptr<hbt::lang::ProgramPart>> tuParts;
	for (auto& pair : devsInFile)
		tuParts.emplace_back(std::unique_ptr<hbt::lang::ProgramPart>(new auto(std::move(pair.second))));

	hbt::lang::TranslationUnit tu(
		std::unique_ptr<std::string>(new auto(path)),
		std::move(tuParts));

	std::unique_ptr<std::ofstream> ofPtr;
	hbt::lang::ASTPrinter(openFile(path, ofPtr)).visit(tu);
}



int main(int argc, char** argv)
{
	po::options_description desc("Usage: hexainfo <IP> [additional options]");
	desc.add_options()
		("help", "produce help message")
		("version", "print version and exit")
		("ip,i", po::value<std::string>(), "IP address of device")
		("interface,I", po::value<std::string>(), "Interface to send multicast from")
		("discover,c", "automatically discover hexabus devices")
		("print,p", "print device and endpoint info to the console")
		("epfile,e", po::value<std::string>(), "path of Hexabus compiler header file to write the endpoint list to")
		("devfile,d", po::value<std::string>(), "path of Hexabus compiler header file to write the device definition to")
		("json,j", po::value<std::string>(), "ouput discovered devices as JSON")
		("verbose,v", "print more status information")
		(",h", "generate human-readable device/endpoint names")
		;

	po::positional_options_description p;
	p.add("ip", 1);

	po::variables_map vm;

	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
		po::notify(vm);
	} catch (std::exception& e) {
		std::cerr << "Cannot process command line: " << e.what() << std::endl;
		exit(-1);
	}

	if (vm.count("help")) {
		std::cout << desc << std::endl;
		return 0;
	}

	if (vm.count("version")) {
		std::cout << "hexinfo -- hexabus endpoint enumeration tool" << std::endl;
		std::cout << "libhexabus version " << hexabus::version() << std::endl;
		return 0;
	}

	try {
		boost::asio::io_service io;
		hexabus::Socket socket(io);
		std::set<boost::asio::ip::address_v6> deviceAddresses;
		NameSanitizer san{vm.count("-h") > 0};

		DiscoverContext dc(socket, boost::posix_time::seconds(2), 3, vm.count("verbose"));

		if (vm.count("interface")) {
			std::string iface = vm["interface"].as<std::string>();
			dc.vout() << "Using network interface " << iface << std::endl;
			socket.mcast_from(iface);
		}

		socket.bind(boost::asio::ip::address_v6::any());
		boost::signals2::scoped_connection c1(socket.onAsyncError([] (const hexabus::GenericException& error) {
			std::cerr << "Network error: " << error.what() << std::endl;
			exit(1);
		}));

		if (vm.count("ip") && vm.count("discover")) {
			std::cerr << "Error: Options --ip and --discover are mutually exclusive." << std::endl;
			return 1;
		} else if (!vm.count("ip") && !vm.count("discover")) {
			std::cerr << "You must either specify an IP address or use the --discover option." << std::endl;
			return 1;
		}

		if (vm.count("ip")) {
			deviceAddresses.insert(boost::asio::ip::address_v6::from_string(vm["ip"].as<std::string>()));
		} else {
			deviceAddresses = enumerateDevices(dc);

			dc.vout() << "Found devices:" << std::endl;
			for (auto addr : deviceAddresses)
				dc.vout() << '\t' << addr << std::endl;
		}

		std::vector<DiscoveredDev> devices;
		std::set<DiscoveredEP> endpoints;
		for (auto addr : deviceAddresses) {
			auto result = queryDevice(dc, addr);
			if (result) {
				endpoints.insert(result->endpoints.begin(), result->endpoints.end());
				devices.push_back(std::move(*result));
			}
		}

		if (vm.count("print")) {
			for (auto& ep : endpoints) {
				printEndpoint(std::cout, ep);
				std::cout << std::endl;
			}

			for (auto& dev : devices) {
				printDevice(std::cout, dev);
				std::cout << std::endl;
			}
		}

		auto absolutePath = [] (const std::string& path) {
			if (path == "-" || boost::filesystem::path(path).is_absolute())
				return path;

			auto buf = get_current_dir_name();
			auto result = boost::filesystem::path(buf) / path;
			free(buf);

			return result.native();
		};

		if (vm.count("epfile"))
			updateEPFile(absolutePath(vm["epfile"].as<std::string>()), devices, san);

		if (vm.count("devfile"))
			updateDevFile(absolutePath(vm["devfile"].as<std::string>()), devices, san);

		if (vm.count("json")) {
			std::unique_ptr<std::ofstream> ofPtr;
			writeDevicesJSON(openFile(vm["json"].as<std::string>(), ofPtr), devices, san);
		}
	} catch (const hexabus::NetworkException& e) {
		std::cerr << "Network error: " << e.code().message() << std::endl;
		return 1;
	}
}
