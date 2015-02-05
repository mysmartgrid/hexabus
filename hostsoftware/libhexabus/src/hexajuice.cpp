#include <functional>
#include <iostream>
#include <set>

#include <libhexabus/socket.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "shared.hpp"

using namespace hexabus;
namespace ba = boost::asio;
namespace bp = boost::property_tree;
namespace bpj = boost::property_tree::json_parser;

namespace std {

// oh dear, boost.
template<int Arg>
struct is_placeholder<boost::arg<Arg>> : integral_constant<int, Arg> {
};

}

namespace {

class FiledescLineWrapper {
private:
	ba::io_service& io;
	ba::posix::stream_descriptor stream;

	std::function<void (const std::string&, bool)> handler;
	std::string buffer, incompleteLine;

	bool processBuffer()
	{
		incompleteLine.reserve(incompleteLine.size() + buffer.size());
		for (auto it = buffer.begin(), end = buffer.end(); it != end; ++it) {
			if (*it == '\n') {
				buffer.erase(buffer.begin(), it + 1);
				auto line = incompleteLine;
				io.post([&, line] () {
					handler(line, false);
				});
				return true;
			} else {
				incompleteLine += *it;
			}
		}
		return false;
	}

	void readSome(const boost::system::error_code& err, size_t size)
	{
		if (err) {
			handler("", true);
			return;
		}

		buffer.resize(size);
		if (!processBuffer())
			continueRead();
	}

	void continueRead()
	{
		buffer.resize(buffer.capacity());
		stream.async_read_some(
			ba::buffer(&buffer[0], buffer.size()),
			std::bind(&FiledescLineWrapper::readSome, this, _1, _2));
	}

	void beginRead()
	{
		incompleteLine.erase();
		if (!processBuffer())
			continueRead();
	}

public:
	FiledescLineWrapper(ba::io_service& io, const ba::posix::stream_descriptor::native_handle_type& handle)
		: io(io), stream(io, handle)
	{
		buffer.reserve(4096);
	}

	~FiledescLineWrapper()
	{
		stream.release();
	}

	void asyncGetline(std::function<void (const std::string&, bool)> handler)
	{
		this->handler = handler;
		beginRead();
	}

	static bool canWrap(const ba::posix::stream_descriptor::native_handle_type& handle)
	{
		ba::io_service io;
		ba::posix::stream_descriptor desc(io);
		boost::system::error_code err;

		desc.assign(handle, err);
		if (err)
			return false;

		desc.release();
		return true;
	}
};



class JsonValue {
public:
	typedef std::shared_ptr<JsonValue> Ptr;

	virtual ~JsonValue() {}
	virtual void write(std::ostream& out) = 0;
};

class JsonString : public JsonValue {
private:
	std::string value;

public:
	JsonString(const std::string& value)
		: value(value)
	{}

	void write(std::ostream& out)
	{
		out << '"';
		for (auto c : value) {
			if (std::isprint(c) && c != '"')
				out << c;
			else
				out << str(boost::format("\\x%02x") % static_cast<unsigned char>(c));
		}
		out << '"';
	}
};

class JsonNonstring : public JsonValue {
private:
	std::string value;

public:
	template<typename T>
	JsonNonstring(T value)
		: value(str(boost::format("%1%") % value))
	{}

	void write(std::ostream& out)
	{
		out << value;
	}
};

class JsonObject : public JsonValue {
private:
	std::map<std::string, Ptr> keys;

public:
	JsonObject(std::map<std::string, Ptr> keys)
		: keys(std::move(keys))
	{
	}

	void write(std::ostream& out)
	{
		unsigned printed = 0;
		out << '{';
		for (const auto& key : keys) {
			if (printed++)
				out << ',';

			JsonString(key.first).write(out);
			out << ':';
			key.second->write(out);
		}
		out << '}';
	}
};

class JsonArray : public JsonValue {
private:
	std::vector<JsonValue::Ptr> values;

public:
	JsonArray(std::vector<JsonValue::Ptr> values)
		: values(std::move(values))
	{
	}

	void write(std::ostream& out)
	{
		unsigned printed = 0;
		out << '[';
		for (const auto& v : values) {
			if (printed++)
				out << ',';
			v->write(out);
		}
		out << ']';
	}
};


JsonValue::Ptr jvalue(const std::string& value)
{
	return JsonValue::Ptr(new JsonString(value));
}

template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
JsonValue::Ptr jvalue(const T& value)
{
	return JsonValue::Ptr(new JsonNonstring(value));
}

template<size_t Len>
JsonValue::Ptr jvalue(const std::array<uint8_t, Len>& array)
{
	std::vector<JsonValue::Ptr> values;
	for (auto c : array)
		values.push_back(jvalue(unsigned(c)));
	return JsonValue::Ptr(new JsonArray(std::move(values)));
}

JsonValue::Ptr jobject(std::map<std::string, JsonValue::Ptr> keys)
{
	return JsonValue::Ptr(new JsonObject(std::move(keys)));
}

std::ostream& operator<<(std::ostream& out, const JsonValue::Ptr& json)
{
	json->write(out);
	return out << std::endl;
}



class PacketFormatter : private PacketVisitor {
private:
	std::map<std::string, JsonValue::Ptr> keys;

	template<typename T>
	void printValuePacket(const char* type, const ValuePacket<T>& packet)
	{
		typedef typename std::conditional<
			std::is_same<T, uint8_t>::value,
			unsigned,
			typename std::conditional<
				std::is_same<T, int8_t>::value,
				int,
				const T&>::type
			>::type Widened;

		keys.insert({ "type", jvalue(type) });
		keys.insert({ "seqnum", jvalue(packet.sequenceNumber()) });
		keys.insert({ "eid", jvalue(packet.eid()) });
		keys.insert({ "flags", jvalue(unsigned(packet.flags())) });
		keys.insert({ "datatype", jvalue(datatypeName((hxb_datatype) packet.datatype())) });
		keys.insert({ "value", jvalue(Widened(packet.value())) });
	}

	template<typename T>
	void printProxyInfoPacket(const char* type, const ProxyInfoPacket<T>& packet)
	{
		printValuePacket(type, packet);
		keys.insert({ "origin", jvalue(packet.origin().to_string()) });
	}

	template<typename T>
	void printReportPacket(const char* type, const ReportPacket<T>& packet)
	{
		printValuePacket(type, packet);
		keys.insert({ "cause", jvalue(packet.cause()) });
	}

	template<typename T>
	void printPropertyReportPacket(const char* type, const PropertyReportPacket<T>& packet)
	{
		printValuePacket(type, packet);
		keys.insert({ "nextid", jvalue(packet.nextid()) });
		keys.insert({ "cause", jvalue(packet.cause()) });
	}

	template<typename T>
	void printPropertyWritePacket(const char* type, const PropertyWritePacket<T>& packet)
	{
		printValuePacket(type, packet);
		keys.insert({ "propid", jvalue(packet.propid()) });
	}

	virtual void visit(const PropertyQueryPacket& propertyQuery)
	{
		keys.insert({ "type", jvalue("propquery") });
		keys.insert({ "seqnum", jvalue(propertyQuery.sequenceNumber()) });
		keys.insert({ "flags", jvalue(unsigned(propertyQuery.flags())) });
		keys.insert({ "eid", jvalue(propertyQuery.eid()) });
		keys.insert({ "eid", jvalue(propertyQuery.eid()) });
		keys.insert({ "propid", jvalue(propertyQuery.propid()) });
	}

	virtual void visit(const ErrorPacket& error)
	{
		keys.insert({ "type", jvalue("error") });
		keys.insert({ "seqnum", jvalue(error.sequenceNumber()) });
		keys.insert({ "flags", jvalue(unsigned(error.flags())) });
		keys.insert({ "code", jvalue(unsigned(error.code())) });
		keys.insert({ "cause", jvalue(unsigned(error.cause())) });
	}

	virtual void visit(const QueryPacket& query)
	{
		keys.insert({ "type", jvalue("query") });
		keys.insert({ "seqnum", jvalue(query.sequenceNumber()) });
		keys.insert({ "flags", jvalue(unsigned(query.flags())) });
		keys.insert({ "eid", jvalue(query.eid()) });
	}

	virtual void visit(const EndpointQueryPacket& endpointQuery)
	{
		keys.insert({ "type", jvalue("epquery") });
		keys.insert({ "seqnum", jvalue(endpointQuery.sequenceNumber()) });
		keys.insert({ "flags", jvalue(unsigned(endpointQuery.flags())) });
		keys.insert({ "eid", jvalue(endpointQuery.eid()) });
	}

	virtual void visit(const EndpointInfoPacket& endpointInfo)
	{
		printValuePacket("epinfo", endpointInfo);
	}

	virtual void visit(const EndpointReportPacket& endpointReport)
	{
		printValuePacket("epreport", endpointReport);
		keys.insert({ "cause", jvalue(endpointReport.cause()) });
	}

	virtual void visit(const AckPacket& ack)
	{
		keys.insert({ "type", jvalue("ack") });
		keys.insert({ "seqnum", jvalue(ack.sequenceNumber()) });
		keys.insert({ "flags", jvalue(unsigned(ack.flags())) });
		keys.insert({ "cause", jvalue(ack.cause()) });
	}

	virtual void visit(const InfoPacket<bool>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<uint8_t>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<uint16_t>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<uint32_t>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<uint64_t>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<int8_t>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<int16_t>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<int32_t>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<int64_t>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<float>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<std::string>& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<std::array<uint8_t, 16> >& info) { printValuePacket("info", info); }
	virtual void visit(const InfoPacket<std::array<uint8_t, 65> >& info) { printValuePacket("info", info); }

	virtual void visit(const ProxyInfoPacket<bool>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<uint8_t>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<uint16_t>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<uint32_t>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<uint64_t>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<int8_t>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<int16_t>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<int32_t>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<int64_t>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<float>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<std::string>& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<std::array<uint8_t, 16> >& pinfo) { printProxyInfoPacket("pinfo", pinfo); }
	virtual void visit(const ProxyInfoPacket<std::array<uint8_t, 65> >& pinfo) { printProxyInfoPacket("pinfo", pinfo); }

	virtual void visit(const WritePacket<bool>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<uint8_t>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<uint16_t>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<uint32_t>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<uint64_t>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<int8_t>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<int16_t>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<int32_t>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<int64_t>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<float>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<std::string>& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<std::array<uint8_t, 16> >& write) { printValuePacket("write", write); }
	virtual void visit(const WritePacket<std::array<uint8_t, 65> >& write) { printValuePacket("write", write); }

	virtual void visit(const ReportPacket<bool>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<uint8_t>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<uint16_t>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<uint32_t>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<uint64_t>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<int8_t>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<int16_t>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<int32_t>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<int64_t>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<float>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<std::string>& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<std::array<uint8_t, 16> >& report) { printReportPacket("report", report); }
	virtual void visit(const ReportPacket<std::array<uint8_t, 65> >& report) { printReportPacket("report", report); }

	virtual void visit(const PropertyReportPacket<bool>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<uint8_t>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<uint16_t>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<uint32_t>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<uint64_t>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<int8_t>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<int16_t>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<int32_t>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<int64_t>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<float>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<std::string>& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<std::array<uint8_t, 16> >& propreport) { printPropertyReportPacket("propreport", propreport); }
	virtual void visit(const PropertyReportPacket<std::array<uint8_t, 65> >& propreport) { printPropertyReportPacket("propreport", propreport); }

	virtual void visit(const PropertyWritePacket<bool>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<uint8_t>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<uint16_t>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<uint32_t>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<uint64_t>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<int8_t>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<int16_t>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<int32_t>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<int64_t>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<float>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<std::string>& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<std::array<uint8_t, 16> >& propwrite) { printPropertyWritePacket("propwrite", propwrite); }
	virtual void visit(const PropertyWritePacket<std::array<uint8_t, 65> >& propwrite) { printPropertyWritePacket("propwrite", propwrite); }

public:
	void print(const Packet& packet, const boost::asio::ip::udp::endpoint& from, const std::string* socket)
	{
		keys.insert({ "from", jobject({
				{ "ip", jvalue(from.address().to_string()) },
				{ "port", jvalue(from.port()) }
			}) });
		if (socket)
			keys.insert({ "socket", jvalue(*socket) });
		packet.accept(*this);
		std::cout << jobject({
			{ "packet", jobject(std::move(keys)) }
		});
	}
};


class Hexajuice {
private:
	ba::io_service& io;
	FiledescLineWrapper& input;

	std::unique_ptr<Listener> listener;
	std::set<std::string> listeningOn;

	std::map<std::string, std::unique_ptr<Socket>> sockets;
	std::map<Socket*, std::string> socketNames;

	struct bad_cast {
		std::string field;
	};

	template<typename To, typename Via = To>
	static To cast(const std::string& field, const std::string& from)
	{
		try {
			auto val = boost::lexical_cast<Via>(from);
			if (val < std::numeric_limits<To>::min() || val > std::numeric_limits<To>::max())
				throw bad_cast{field};
			return val;
		} catch (const boost::bad_lexical_cast& e) {
			throw bad_cast{field};
		}
	}

	template<size_t Len>
	static std::array<uint8_t, Len> parseBinary(const bp::ptree& value)
	{
		std::array<uint8_t, Len> result;

		if (value.size() != Len)
			throw bad_cast{"packet.value"};

		auto it = value.begin();
		for (size_t i = 0; i < Len; i++, it++)
			result[i] = cast<uint8_t, unsigned>("packet.value", it->second.data());

		return result;
	}

	struct makeInfo {
		template<typename Value>
		static std::unique_ptr<Packet> run(uint32_t eid, const Value& value, uint8_t flags)
		{
			return std::unique_ptr<Packet>(new InfoPacket<Value>(eid, value, flags));
		}
	};

	struct makeWrite {
		template<typename Value>
		static std::unique_ptr<Packet> run(uint32_t eid, const Value& value, uint8_t flags)
		{
			return std::unique_ptr<Packet>(new WritePacket<Value>(eid, value, flags));
		}
	};

	struct makeReport {
		template<typename Value>
		static std::unique_ptr<Packet> run(uint32_t eid, const Value& value, uint8_t flags, uint16_t cause)
		{
			return std::unique_ptr<Packet>(new ReportPacket<Value>(cause, eid, value, flags));
		}
	};

	struct makeProxyInfo {
		template<typename Value>
		static std::unique_ptr<Packet> run(uint32_t eid, const Value& value, uint8_t flags, boost::asio::ip::address_v6 origin)
		{
			return std::unique_ptr<Packet>(new ProxyInfoPacket<Value>(origin, eid, value, flags));
		}
	};

	struct makePropertyWrite {
		template<typename Value>
		static std::unique_ptr<Packet> run(uint32_t eid, const Value& value, uint8_t flags, uint32_t propid)
		{
			return std::unique_ptr<Packet>(new PropertyWritePacket<Value>(propid, eid, value, flags));
		}
	};

	struct makePropertyReport {
		template<typename Value>
		static std::unique_ptr<Packet> run(uint32_t eid, const Value& value, uint8_t flags, uint32_t cause, uint16_t nextid)
		{
			return std::unique_ptr<Packet>(new PropertyReportPacket<Value>(nextid, cause, eid, value, flags));
		}
	};

	template<class Make, typename... Extra>
	static std::unique_ptr<Packet> parseValuePacket(uint32_t eid, uint8_t dt, const bp::ptree& value,
			uint8_t flags, const Extra&... extra)
	{
		switch (dt) {
		case HXB_DTYPE_BOOL:
			return Make::run(eid, cast<bool>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_UINT8:
			return Make::run(eid, cast<uint8_t, unsigned>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_UINT16:
			return Make::run(eid, cast<uint16_t>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_UINT32:
			return Make::run(eid, cast<uint32_t>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_UINT64:
			return Make::run(eid, cast<uint64_t>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_SINT8:
			return Make::run(eid, cast<int8_t, int>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_SINT16:
			return Make::run(eid, cast<int16_t>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_SINT32:
			return Make::run(eid, cast<int32_t>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_SINT64:
			return Make::run(eid, cast<int64_t>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_FLOAT:
			return Make::run(eid, cast<float>("packet.value", value.data()), flags, extra...);
		case HXB_DTYPE_128STRING:
			if (value.data().size() > ValuePacket<std::string>::max_length)
				throw bad_cast{"packet.value"};
			return Make::run(eid, value.data(), flags, extra...);
		case HXB_DTYPE_65BYTES:
			return Make::run(eid, parseBinary<65>(value), flags, extra...);
		case HXB_DTYPE_16BYTES:
			return Make::run(eid, parseBinary<16>(value), flags, extra...);
		}

		return nullptr;
	}

	void processPacket(const Packet& packet, const boost::asio::ip::udp::endpoint& from, Socket* socket)
	{
		const auto* socketName = socketNames.count(socket) ? &socketNames.at(socket) : nullptr;
		PacketFormatter().print(packet, from, socketName);
	}

	void processTransmission(const Packet& packet, uint16_t seqNum, const boost::asio::ip::udp::endpoint& from, bool failed)
	{
		if(failed) {
			std::cout << jobject({
				{ "error", jobject({
						{ "type", jvalue("reliable transmission failed") },
						{ "seqnum", jvalue(seqNum) },
						{ "diag", jvalue("ack timeout") }
					}) }
			});
		} else {
			std::cout << jobject({
				{ "info", jobject({
						{ "seqnum", jvalue(seqNum) },
						{ "message", jvalue("sent reliable packet") },
					}) }
			});
		}
	}

	Socket& openSocket(const std::string& name)
	{
		std::unique_ptr<Socket> sock(new Socket(io));
		sock->onPacketReceived(std::bind(&Hexajuice::processPacket, this, _1, _2, sock.get()));
		auto it = sockets.emplace(name, std::move(sock)).first;
		socketNames.insert({ it->second.get(), name });
		return *it->second;
	};

	void sendReliable(Socket& sock, const Packet& pack, const boost::asio::ip::udp::endpoint& dest)
	{
		sock.onPacketTransmitted(std::bind(&Hexajuice::processTransmission, this, _1, _2, _3, _4),
			pack, dest);
	}

	void processCommand(const bp::ptree& json)
	{
		auto requiredField = [&] (const std::string& field) -> const bp::ptree& {
			auto child = json.get_child_optional(field);
			if (!child) {
				throw jobject({
					{ "error", jobject({
							{ "type", jvalue("required field missing") },
							{ "field", jvalue(field) }
						}) }
				});
			}
			return *child;
		};

		auto parseAddress = [&] (const std::string& prefix, uint16_t defaultPort) -> ba::ip::udp::endpoint {
			boost::system::error_code err;
			auto& addr = requiredField(prefix);

			if (addr.size() == 0) {
				auto result = resolve(io, addr.data(), err);
				if (err)
					throw boost::system::system_error(err);

				return {result, defaultPort};
			} else {
				auto ip = resolve(io, requiredField(prefix + ".ip").data(), err);
				auto port = cast<uint16_t>(prefix + ".port", requiredField(prefix + ".port").data());

				if (err)
					throw boost::system::system_error(err);

				return {ip, port};
			}
		};

		auto parseSocket = [&] () -> Socket& {
			auto socket = json.get_child_optional("socket");

			if (!socket) {
				if (!sockets.size())
					return openSocket("");
				if (sockets.size() == 1)
					return *sockets.begin()->second;
			} else {
				auto resolved = sockets.find(socket->data());
				if (resolved != sockets.end())
					return *resolved->second;
				else
					throw jobject({
						{ "error", jobject({
								{ "type", jvalue("invalid input") },
								{ "diag", jvalue("unknown socket") }
							}) }
					});
			}

			throw jobject({
				{ "error", jobject({
						{ "type", jvalue("invalid input") },
						{ "diag", jvalue("socket not specified") }
					}) }
			});
		};

		auto parsePacket = [&] () {
			auto& type = requiredField("packet.type").data();
			auto flags = json.get("packet.flags", 0);

			if (type == "error") {
				auto code = cast<uint8_t>("packet.code", requiredField("packet.code").data());
				return std::unique_ptr<Packet>(new ErrorPacket(code, flags));
			} else if (type == "ack") {
				auto cause = cast<uint16_t>("packet.cause", requiredField("packet.cause").data());
				return std::unique_ptr<Packet>(new AckPacket(cause, flags));
			} else if (type == "query") {
				auto eid = cast<uint32_t>("packet.eid", requiredField("packet.eid").data());
				return std::unique_ptr<Packet>(new QueryPacket(eid, flags));
			} else if (type == "epquery") {
				auto eid = cast<uint32_t>("packet.eid", requiredField("packet.eid").data());
				return std::unique_ptr<Packet>(new EndpointQueryPacket(eid, flags));
			} else if (type == "epinfo") {
				auto eid = cast<uint32_t>("packet.eid", requiredField("packet.eid").data());
				auto dt = cast<uint8_t>("packet.datatype", requiredField("packet.datatype").data());
				auto value = requiredField("packet.value").data();
				return std::unique_ptr<Packet>(new EndpointInfoPacket(eid, dt, value, flags));
			} else if (type == "epreport") {
				auto eid = cast<uint32_t>("packet.eid", requiredField("packet.eid").data());
				auto dt = cast<uint8_t>("packet.datatype", requiredField("packet.datatype").data());
				auto value = requiredField("packet.value").data();
				auto cause = cast<uint16_t>("packet.cause", requiredField("packet.cause").data());
				return std::unique_ptr<Packet>(new EndpointReportPacket(cause ,eid, dt, value, flags));
			} else if (type == "propquery"){
				auto eid = cast<uint32_t>("packet.eid", requiredField("packet.eid").data());
				auto propid = cast<uint32_t>("packet.propid", requiredField("packet.propid").data());
				return std::unique_ptr<Packet>(new PropertyQueryPacket(propid, eid, flags));
			} else if (type == "info" || type == "report" || type == "write" || type == "pinfo" ||
				type == "propwrite" || type == "propreport")
			{
				auto eid = cast<uint32_t>("packet.eid", requiredField("packet.eid").data());
				auto dt = dtypeStrToDType(requiredField("packet.datatype").data());
				auto value = requiredField("packet.value");

				if (dt < 0)
					throw jobject({
						{ "error", jobject({
								{ "type", jvalue("invalid input") },
								{ "diag", jvalue("invalid datatype") }
							}) }
					});

				if (type == "report") {
					auto cause = cast<uint16_t>("packet.cause", requiredField("packet.cause").data());
					return parseValuePacket<makeReport>(eid, dt, value, flags, cause);
				} else if (type == "write") {
					return parseValuePacket<makeWrite>(eid, dt, value, flags);
				} else if (type == "pinfo") {
					auto originStr = requiredField("packet.origin").data();
					boost::asio::ip::address_v6 originAddr;
					boost::system::error_code err;

					originAddr = boost::asio::ip::address_v6::from_string(originStr, err);
					if(err)
						throw boost::system::error_code(err);

					return parseValuePacket<makeProxyInfo>(eid, dt, value, flags, originAddr);
				} else if (type == "propwrite") {
					auto propid = cast<uint32_t>("packet.propid", requiredField("packet.propid").data());
					return parseValuePacket<makePropertyWrite>(eid, dt, value, flags, propid);
				} else if (type == "propreport") {
					auto nextid = cast<uint32_t>("packet.nextid", requiredField("packet.nextid").data());
					auto cause = cast<uint16_t>("packet.cause", requiredField("packet.cause").data());
					return parseValuePacket<makePropertyReport>(eid, dt, value, flags, cause, nextid);
				} else {
					return parseValuePacket<makeInfo>(eid, dt, value, flags);
				}
			} else {
				throw jobject({
					{ "error", jobject({
							{ "type", jvalue("invalid input") },
							{ "diag", jvalue("invalid packet type") }
						}) }
				});
			}
		};

		auto command = requiredField("command").data();

		if (command == "listen") {
			auto& iface = requiredField("interface");
			if (!listener) {
				listener.reset(new Listener(io));
				listener->onPacketReceived(std::bind(&Hexajuice::processPacket, this, _1, _2, nullptr));
			}
			listeningOn.insert(iface.data());
			listener->listen(iface.data());
		} else if (command == "ignore") {
			auto& iface = requiredField("interface");
			listener->ignore(iface.data());
			listeningOn.erase(iface.data());
			if (!listeningOn.size())
				listener.reset();
		} else if (command == "open") {
			auto name = requiredField("socket").data();
			if (sockets.count(name))
				throw jobject({
					{ "error", jobject({
						{ "type", jvalue("invalid input") },
						{ "diag", jvalue("socket name already used") }
					}) }
				});
			openSocket(name);
		} else if (command == "close") {
			auto name = requiredField("socket").data();
			auto it = sockets.find(name);
			if (it == sockets.end())
				throw jobject({
					{ "error", jobject({
						{ "type", jvalue("invalid input") },
						{ "diag", jvalue("unknown socket") }
					}) }
				});
			socketNames.erase(it->second.get());
			sockets.erase(it);
		} else if (command == "mcast_from") {
			auto& iface = requiredField("interface");
			parseSocket().mcast_from(iface.data());
		} else if (command == "bind") {
			parseSocket().bind(parseAddress("address", 0));
		} else if (command == "send") {
			auto packet = parsePacket();
			ba::ip::udp::endpoint target{Socket::GroupAddress, 61616};

			if (json.get_child_optional("to"))
				target = parseAddress("to", 61616);

			if (packet->flags()&HXB_FLAG_RELIABLE) {
				sendReliable(parseSocket(), *packet, target);
			} else {
				std::cout << jobject({
					{ "info", jobject({
							{ "seqnum", jvalue(parseSocket().send(*packet, target)) },
							{ "message", jvalue("sent packet") },
						}) }
				});
			}
		} else if (command == "quit") {
			exit(0);
		}
	}

	void onCommand(const std::string& line, bool eof)
	{
		if (eof) {
			io.stop();
			return;
		}

		allowNextCommand();

		bp::ptree json;

		try {
			std::istringstream stream(line);
			bpj::read_json(stream, json);
		} catch (const bpj::json_parser_error& e) {
			std::cout << jobject({
				{ "error", jobject({
						{ "type", jvalue("invalid input") },
						{ "diag", jvalue(e.message()) }
					}) }
			});
			return;
		}

		try {
			processCommand(json);
		} catch (const JsonValue::Ptr& err) {
			std::cout << err;
		} catch (const NetworkException& e) {
			std::cout << jobject({
				{ "error", jobject({
						{ "type", jvalue("system error") },
						{ "category", jvalue(e.code().category().name()) },
						{ "code", jvalue(e.code().value()) },
						{ "message", jvalue(e.code().message()) }
					}) }
			});
		} catch (const bad_cast& e) {
			std::cout << jobject({
				{ "error", jobject({
						{ "type", jvalue("invalid input") },
						{ "diag", jvalue("invalid input value") },
						{ "field", jvalue(e.field) }
					}) }
			});
		} catch (const boost::system::system_error& e) {
			std::cout << jobject({
				{ "error", jobject({
						{ "type", jvalue("system error") },
						{ "category", jvalue(e.code().category().name()) },
						{ "code", jvalue(e.code().value()) },
						{ "message", jvalue(e.code().message()) }
					}) }
			});
		} catch (const std::exception& e) {
			std::cout << jobject({
				{ "error", jobject({
						{ "type", jvalue("unknown") },
						{ "diag", jvalue(e.what()) }
					}) }
			});
		} catch (...) {
			std::cout << jobject({
				{ "error", jobject({
						{ "type", jvalue("unknown") }
					}) }
			});
		}
	}

	void allowNextCommand()
	{
		input.asyncGetline(std::bind(&Hexajuice::onCommand, this, _1, _2));
	}

public:
	Hexajuice(ba::io_service& io, FiledescLineWrapper& input)
		: io(io), input(input)
	{
		allowNextCommand();
	}
};

}

int main(int argc, char* argv[])
{
	if (argc != 1) {
		std::cout << R"(Usage: hexajuice)" << std::endl;
		return 0;
	}

	if (!FiledescLineWrapper::canWrap(STDIN_FILENO)) {
		std::cerr << "stdin is not a stream" << std::endl;
		return 1;
	}

	try {
		ba::io_service io;
		FiledescLineWrapper input(io, STDIN_FILENO);
		Hexajuice juice(io, input);

		io.run();
	} catch (const std::exception& e) {
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	} catch (...) {
		std::cerr << "unknown error occured" << std::endl;
		return 1;
	}

	return 0;
}
