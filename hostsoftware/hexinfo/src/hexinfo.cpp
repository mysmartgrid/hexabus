#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include <boost/variant.hpp>
#include <libhexabus/common.hpp>
#include <libhexabus/packet.hpp>
#include <libhexabus/error.hpp>
#include <libhexabus/socket.hpp>
#include <libhexabus/endpoint_registry.hpp>
#include <hbt/Lang/ast.hpp>
#include <hbt/Lang/astprinter.hpp>
#include <hbt/Lang/parser.hpp>

namespace po = boost::program_options;

namespace {

struct DiscoveredProperty {
	uint32_t propid;
	hexabus::hxb_datatype type;
	boost::variant<
		bool,
		uint8_t,
		uint16_t,
		uint32_t,
		uint64_t,
		int8_t,
		int16_t,
		int32_t,
		int64_t,
		float,
		std::string
	> value;

	bool operator<(const DiscoveredProperty& other) const { return propid < other.propid; }
};

struct DiscoveredEP {
	uint32_t eid;
	hexabus::hxb_datatype type;
	std::string name;
	std::vector<DiscoveredProperty> properties;

	bool operator<(const DiscoveredEP& other) const { return eid < other.eid; }
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
		std::stringstream result;
		bool empty = true;

		for (auto c : name) {
			if (isalnum(c) && (empty || isalpha(c))) {
				result << c;
				empty = false;
				continue;
			}

			result << '_';
			result << std::setw(2) << std::hex << int((unsigned char)c);
			empty = false;
		}

		return result.str();
	}

	std::string sanitizeName(const DiscoveredEP& ep)
	{
		if (forHumans)
			return sanitizeString(ep.name) + "_EP";

		std::stringstream result;
		result << "ep_" << ep.eid;

		return result.str();
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

static std::set<std::pair<boost::asio::ip::address_v6, std::string> > enumerateDevices(DiscoverContext& dc)
{
	namespace hf = hexabus::filtering;

	std::set<std::pair<boost::asio::ip::address_v6, std::string> > addresses;

	auto response = [&] (const hexabus::Packet& packet, boost::asio::ip::udp::endpoint ep) {
		auto name = static_cast<const hexabus::ValuePacket<std::string>&>(packet).value();
		addresses.insert(make_pair(ep.address().to_v6(), name));
		return false;
	};

	dc.vout() << "Sending discovery packets ";
	dc.retryPacket(hexabus::QueryPacket(0), dc.socket.GroupAddress, response, hf::isReport<std::string>() && (hf::eid() == 0UL));
	dc.vout() << std::endl;

	return addresses;
}

static std::pair<boost::asio::ip::address_v6, std::string> getDeviceName(DiscoverContext& dc, boost::asio::ip::address_v6 ip)
{
	namespace hf = hexabus::filtering;

	std::pair<boost::asio::ip::address_v6, std::string> address;
	boost::signals2::scoped_connection sc;

	auto transmission_response = [&] (const hexabus::Packet& packet, uint16_t seqNum, const boost::asio::ip::udp::endpoint& from, bool transmissionFailed) {
		if (transmissionFailed) {

			std::cerr << "Reply timeout." << std::endl;
			sc.disconnect();
			dc.socket.ioService().stop();
			exit(1);
		}
	};

	auto response = [&] (const hexabus::Packet& packet, boost::asio::ip::udp::endpoint ep) {
		auto name = static_cast<const hexabus::ValuePacket<std::string>&>(packet).value();
		address = make_pair(ep.address().to_v6(), name);

		sc.disconnect();
		dc.socket.ioService().stop();
	};

	dc.vout() << "Sending discovery packet ";
	sc = dc.socket.onPacketReceived(response, hf::isReport<std::string>() && (hf::eid() == 0UL));
	dc.socket.onPacketTransmitted(transmission_response, hexabus::QueryPacket(0, hexabus::HXB_FLAG_WANT_ACK||hexabus::HXB_FLAG_RELIABLE), boost::asio::ip::udp::endpoint(ip, 61616));
	dc.vout() << std::endl;

	dc.socket.ioService().reset();
	dc.socket.ioService().run();

	return address;
}

static uint32_t addProperty(const hexabus::Packet& packet, DiscoveredEP& ep, uint32_t propid)
{
	do {
		const hexabus::PropertyReportPacket<bool>* propertyReport_b = dynamic_cast<const hexabus::PropertyReportPacket<bool>* >(&packet);
			if(propertyReport_b !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_b->datatype()),
					propertyReport_b->value()
				});
				return propertyReport_b->nextid();
			}
		const hexabus::PropertyReportPacket<uint8_t>* propertyReport_u8 = dynamic_cast<const hexabus::PropertyReportPacket<uint8_t>* >(&packet);
			if(propertyReport_u8 !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_u8->datatype()),
					propertyReport_u8->value()
				});
				return propertyReport_u8->nextid();
			}
		const hexabus::PropertyReportPacket<uint16_t>* propertyReport_u16 = dynamic_cast<const hexabus::PropertyReportPacket<uint16_t>* >(&packet);
			if(propertyReport_u16 !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_u16->datatype()),
					propertyReport_u16->value()
				});
				return propertyReport_u16->nextid();
			}
		const hexabus::PropertyReportPacket<uint32_t>* propertyReport_u32 = dynamic_cast<const hexabus::PropertyReportPacket<uint32_t>* >(&packet);
			if(propertyReport_u32 !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_u32->datatype()),
					propertyReport_u32->value()
				});
				return propertyReport_u32->nextid();
			}
		const hexabus::PropertyReportPacket<uint64_t>* propertyReport_u64 = dynamic_cast<const hexabus::PropertyReportPacket<uint64_t>* >(&packet);
			if(propertyReport_u64 !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_u64->datatype()),
					propertyReport_u64->value()
				});
				return propertyReport_u64->nextid();
			}
		const hexabus::PropertyReportPacket<int8_t>* propertyReport_s8 = dynamic_cast<const hexabus::PropertyReportPacket<int8_t>* >(&packet);
			if(propertyReport_s8 !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_s8->datatype()),
					propertyReport_s8->value()
				});
				return propertyReport_s8->nextid();
			}
		const hexabus::PropertyReportPacket<int16_t>* propertyReport_s16 = dynamic_cast<const hexabus::PropertyReportPacket<int16_t>* >(&packet);
			if(propertyReport_s16 !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_s16->datatype()),
					propertyReport_s16->value()
				});
				return propertyReport_s16->nextid();
			}
		const hexabus::PropertyReportPacket<int32_t>* propertyReport_s32 = dynamic_cast<const hexabus::PropertyReportPacket<int32_t>* >(&packet);
			if(propertyReport_s32 !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_s32->datatype()),
					propertyReport_s32->value()
				});
				return propertyReport_s32->nextid();
			}
		const hexabus::PropertyReportPacket<int64_t>* propertyReport_s64 = dynamic_cast<const hexabus::PropertyReportPacket<int64_t>* >(&packet);
			if(propertyReport_s64 !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_s64->datatype()),
					propertyReport_s64->value()
				});
				return propertyReport_s64->nextid();
			}
		const hexabus::PropertyReportPacket<float>* propertyReport_f = dynamic_cast<const hexabus::PropertyReportPacket<float>* >(&packet);
			if(propertyReport_f !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_f->datatype()),
					propertyReport_f->value()
				});
				return propertyReport_f->nextid();
			}
		const hexabus::PropertyReportPacket<std::string>* propertyReport_str = dynamic_cast<const hexabus::PropertyReportPacket<std::string>* >(&packet);
			if(propertyReport_str !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_str->datatype()),
					propertyReport_str->value()
				});
				return propertyReport_str->nextid();
			}
		const hexabus::PropertyReportPacket<std::array<uint8_t, 16> >* propertyReport_16b = dynamic_cast<const hexabus::PropertyReportPacket<std::array<uint8_t, 16> >* >(&packet);
			if(propertyReport_16b !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_16b->datatype()),
					std::string(propertyReport_16b->value().begin(), propertyReport_16b->value().end())
				});
				return propertyReport_16b->nextid();
			}
		const hexabus::PropertyReportPacket<std::array<uint8_t, 65> >* propertyReport_65b = dynamic_cast<const hexabus::PropertyReportPacket<std::array<uint8_t, 65> >* >(&packet);
			if(propertyReport_65b !=NULL) {
				ep.properties.push_back ({
					propid,
					hexabus::hxb_datatype(propertyReport_65b->datatype()),
					std::string(propertyReport_65b->value().begin(), propertyReport_65b->value().end())
				});
				return propertyReport_65b->nextid();
			}
	} while(false);

	return 0;
}

static uint32_t queryProperty(DiscoverContext& dc, DiscoveredDev& device, uint32_t eid, uint32_t propid)
{
	namespace hf = hexabus::filtering;

	uint32_t nextpropid;
	boost::signals2::scoped_connection sc;

	auto transmission_response = [&] (const hexabus::Packet& packet, uint16_t seqNum, const boost::asio::ip::udp::endpoint& from, bool transmissionFailed) {
		if (transmissionFailed) {

			std::cerr << "Reply timeout." << std::endl;
			sc.disconnect();
			dc.socket.ioService().stop();
			exit(1);
		}
	};
	auto response = [&] (const hexabus::Packet& packet, boost::asio::ip::udp::endpoint ep) {
		for (auto& ep : device.endpoints) {
				if(ep.eid == eid) {
					nextpropid = addProperty(packet, ep, propid);
					break;
				}
		}

		sc.disconnect();
		dc.socket.ioService().stop();
		return true;
	};

	dc.vout() << "Querying Property " << propid << std::endl;
	sc = dc.socket.onPacketReceived(response,
		hf::isPropertyReport<bool>() ||
		hf::isPropertyReport<uint8_t>() ||
		hf::isPropertyReport<uint16_t>() ||
		hf::isPropertyReport<uint32_t>() ||
		hf::isPropertyReport<uint64_t>() ||
		hf::isPropertyReport<int8_t>() ||
		hf::isPropertyReport<int16_t>() ||
		hf::isPropertyReport<int32_t>() ||
		hf::isPropertyReport<int64_t>() ||
		hf::isPropertyReport<float>() ||
		hf::isPropertyReport<std::string>() ||
		hf::isPropertyReport<std::array<uint8_t, 16> >() ||
		hf::isPropertyReport<std::array<uint8_t, 65> >()
	);
	dc.socket.onPacketTransmitted(transmission_response, hexabus::PropertyQueryPacket(propid, eid, hexabus::HXB_FLAG_WANT_ACK||hexabus::HXB_FLAG_RELIABLE),
			boost::asio::ip::udp::endpoint(device.address, 61616));

	dc.socket.ioService().reset();
	dc.socket.ioService().run();

	return nextpropid;
}

static uint32_t queryEndpoint(DiscoverContext& dc, DiscoveredDev& device, uint32_t eid)
{
	namespace hf = hexabus::filtering;

	uint32_t propid = 0;
	boost::signals2::scoped_connection sc;

	auto transmission_response = [&] (const hexabus::Packet& packet, uint16_t seqNum, const boost::asio::ip::udp::endpoint& from, bool transmissionFailed) {
		if (transmissionFailed) {

			std::cerr << "Reply timeout." << std::endl;
			sc.disconnect();
			dc.socket.ioService().stop();
			exit(1);
		}
	};

	auto response = [&] (const hexabus::Packet& packet, boost::asio::ip::udp::endpoint ep) {
		auto& report = dynamic_cast<const hexabus::EndpointReportPacket&>(packet);

		device.endpoints.push_back({
				eid,
				hexabus::hxb_datatype(report.datatype()),
				report.value(),
				{}
		});

		sc.disconnect();
		dc.socket.ioService().stop();
	};

	dc.vout() << "Querying Endpoint " << eid << std::endl;
	sc = dc.socket.onPacketReceived(response, hf::isEndpointReport());
	dc.socket.onPacketTransmitted(transmission_response, hexabus::EndpointQueryPacket(eid, hexabus::HXB_FLAG_WANT_ACK||hexabus::HXB_FLAG_RELIABLE),
			boost::asio::ip::udp::endpoint(device.address, 61616));

	dc.socket.ioService().reset();
	dc.socket.ioService().run();

	do {
		propid = queryProperty(dc, device, eid, propid);
	} while(propid != 0);

	dc.vout() << std::endl;

	if(!device.endpoints.empty() && !device.endpoints.back().properties.empty() &&
		device.endpoints.back().properties.front().propid==0) {

		return boost::get<uint32_t>(device.endpoints.back().properties.front().value);
	} else {
		return 0;
	}
}

static boost::optional<DiscoveredDev> queryDevice(DiscoverContext& dc, std::pair<boost::asio::ip::address_v6, std::string> addr)
{
	uint32_t eid = 0;

	DiscoveredDev result = {
		addr.first,
		addr.second,
		{}
	};

	dc.vout() << "Querying device " << addr.first << '\t' << addr.second << std::endl;

	do {
		eid = queryEndpoint(dc, result, eid);
	} while(eid != 0);

	return result;
}



static void printEndpoint(std::ostream& out, const DiscoveredEP& ep)
{
	out << "Endpoint information:" << std::endl
		<< "\tEndpoint ID:\t" << ep.eid << std::endl
		<< "\tName:       \t" << ep.name << std::endl
		<< "\tDatatype:   \t" << hexabus::datatypeName(ep.type) << std::endl;
	out << std::endl;
}

static void printProperty(std::ostream& out, const DiscoveredProperty& prop)
{
	out << "\tProperty information:" << std::endl
		<< "\t\tProperty ID: \t" << prop.propid << std::endl
		<< "\t\tDatatype:    \t" << hexabus::datatypeName(prop.type) << std::endl;
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

	out << "\tProperties:" << std::endl;

	for (auto& ep : dev.endpoints) {
		out << "\t\tEndpoint " << ep.eid << ":" << std::endl;
		for (auto& prop : ep.properties) {
			out << "\t\t\tID " << prop.propid << ":\t" << prop.value << std::endl;
		}
	}
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
		if (put++)
			out << ", ";

		out
			<< R"({)" << std::endl
			<< R"(			"eid": )" << ep.eid << ',' << std::endl
			<< R"(			"sm_name": ")" << san.sanitizeName(ep) << R"(",)" << std::endl
			<< R"(			"type": ")" << hexabus::datatypeName(ep.type) << R"(",)" << std::endl;

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
			out << R"(",)" << std::endl;
			out
				<< R"(			"properties": [)" << std::endl
				<< R"(				)";

			unsigned putp = 0;
			for(auto& prop : ep.properties) {
				if (putp++)
					out << ", ";

				out
					<< R"({)" << std::endl
					<< R"(					"propid:": )" << prop.propid << ',' << std::endl
					<< R"(					"type:": ")" << hexabus::datatypeName(prop.type) << R"(",)"<< std::endl;
				if(prop.type == hexabus::HXB_DTYPE_128STRING || prop.type == hexabus::HXB_DTYPE_16BYTES || prop.type == hexabus::HXB_DTYPE_65BYTES)
					out << R"(					"value:": ")" << san.sanitizeString(boost::get<std::string>(prop.value)) << R"(")" << std::endl;
				else
					out << R"(					"value:": )" << prop.value << std::endl;
				out << "				}";
			}
		}
		out << ']' << std::endl << "		}";
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


static bool hasHbtType(hexabus::hxb_datatype type) {
	switch (type) {
	case hexabus::HXB_DTYPE_BOOL:
	case hexabus::HXB_DTYPE_UINT8:
	case hexabus::HXB_DTYPE_UINT16:
	case hexabus::HXB_DTYPE_UINT32:
	case hexabus::HXB_DTYPE_UINT64:
	case hexabus::HXB_DTYPE_SINT8:
	case hexabus::HXB_DTYPE_SINT16:
	case hexabus::HXB_DTYPE_SINT32:
	case hexabus::HXB_DTYPE_SINT64:
	case hexabus::HXB_DTYPE_FLOAT: return true; break;
	default: return false;
	}
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
			if (!hasHbtType(ep.type))
				continue;

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
			default: break;
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
		devsInFile.erase(san.sanitizeName(dev));

		auto addrBytes = dev.address.to_bytes();
		std::array<uint8_t, 16> address;
		std::copy(addrBytes.begin(), addrBytes.end(), address.begin());

		std::vector<hbt::lang::Identifier> endpoints;
		for (auto& ep : dev.endpoints) {
			if(hasHbtType(ep.type)) {
				endpoints.emplace_back(noSloc, san.sanitizeName(ep));
			}
		}

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
		hexabus::Socket socket(io, 5);
		std::set<std::pair<boost::asio::ip::address_v6, std::string> > deviceAddresses;
		NameSanitizer san{vm.count("-h") > 0};

		DiscoverContext dc(socket, boost::posix_time::seconds(2), 5, vm.count("verbose"));

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
			deviceAddresses.insert(getDeviceName(dc, boost::asio::ip::address_v6::from_string(vm["ip"].as<std::string>())));
		} else {
			deviceAddresses = enumerateDevices(dc);

			dc.vout() << "Found devices:" << std::endl;
			for (auto addr : deviceAddresses)
				dc.vout() << '\t' << addr.first << "\t\t" << addr.second << std::endl;
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

				for(auto& prop : ep.properties) {
					printProperty(std::cout, prop);
				}
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
