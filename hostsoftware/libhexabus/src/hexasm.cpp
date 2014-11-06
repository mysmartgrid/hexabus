#include <iostream>
#include <utility>
#include <vector>

#include <string.h>

#include "libhexabus/sm/machine.hpp"
#include "libhexabus/socket.hpp"

using namespace hexabus;
using namespace hexabus::sm;

class OnPacketVisitor : private PacketVisitor {
private:
	hexabus::sm::Machine& machine;
	const boost::asio::ip::udp::endpoint& remote;

public:
	OnPacketVisitor(hexabus::sm::Machine& machine, const boost::asio::ip::udp::endpoint& remote)
		: machine(machine), remote(remote)
	{}

	void run(const hexabus::Packet& p)
	{
		p.accept(*this);
	}

private:
	template<typename Part, Part hxb_sm_value_t::*Val, typename T>
	void run(const hexabus::InfoPacket<T>& p)
	{
		boost::asio::ip::address_v6::bytes_type addr;
		addr = remote.address().to_v6().to_bytes();

		hxb_sm_value_t value;

		value.type = p.datatype();
		value.*Val = p.value();

		std::cout << "Running state machine for packet: "
			<< machine.run_sm((const char*) &addr[0], p.eid(), &value)
			<< std::endl;
	}

	virtual void visit(const hexabus::ErrorPacket& error) {}
	virtual void visit(const hexabus::QueryPacket& query) {}
	virtual void visit(const hexabus::EndpointQueryPacket& endpointQuery) {}
	virtual void visit(const hexabus::EndpointInfoPacket& endpointInfo) {}

	virtual void visit(const hexabus::InfoPacket<bool>& info) { run<uint32_t, &hxb_sm_value_t::v_uint>(info); }
	virtual void visit(const hexabus::InfoPacket<uint8_t>& info) { run<uint32_t, &hxb_sm_value_t::v_uint>(info); }
	virtual void visit(const hexabus::InfoPacket<uint16_t>& info) { run<uint32_t, &hxb_sm_value_t::v_uint>(info); }
	virtual void visit(const hexabus::InfoPacket<uint32_t>& info) { run<uint32_t, &hxb_sm_value_t::v_uint>(info); }
	virtual void visit(const hexabus::InfoPacket<uint64_t>& info) { run<uint64_t, &hxb_sm_value_t::v_uint64>(info); }
	virtual void visit(const hexabus::InfoPacket<int8_t>& info) { run<int32_t, &hxb_sm_value_t::v_sint>(info); }
	virtual void visit(const hexabus::InfoPacket<int16_t>& info) { run<int32_t, &hxb_sm_value_t::v_sint>(info); }
	virtual void visit(const hexabus::InfoPacket<int32_t>& info) { run<int32_t, &hxb_sm_value_t::v_sint>(info); }
	virtual void visit(const hexabus::InfoPacket<int64_t>& info) { run<int64_t, &hxb_sm_value_t::v_sint64>(info); }
	virtual void visit(const hexabus::InfoPacket<float>& info) { run<float, &hxb_sm_value_t::v_float>(info); }
	virtual void visit(const hexabus::InfoPacket<std::string>& info) {}
	virtual void visit(const hexabus::InfoPacket<boost::array<char, 16> >& info) {}
	virtual void visit(const hexabus::InfoPacket<boost::array<char, 65> >& info) {}

	virtual void visit(const hexabus::WritePacket<bool>& write) {}
	virtual void visit(const hexabus::WritePacket<uint8_t>& write) {}
	virtual void visit(const hexabus::WritePacket<uint16_t>& write) {}
	virtual void visit(const hexabus::WritePacket<uint32_t>& write) {}
	virtual void visit(const hexabus::WritePacket<uint64_t>& write) {}
	virtual void visit(const hexabus::WritePacket<int8_t>& write) {}
	virtual void visit(const hexabus::WritePacket<int16_t>& write) {}
	virtual void visit(const hexabus::WritePacket<int32_t>& write) {}
	virtual void visit(const hexabus::WritePacket<int64_t>& write) {}
	virtual void visit(const hexabus::WritePacket<float>& write) {}
	virtual void visit(const hexabus::WritePacket<std::string>& write) {}
	virtual void visit(const hexabus::WritePacket<boost::array<char, 16> >& write) {}
	virtual void visit(const hexabus::WritePacket<boost::array<char, 65> >& write) {}
};

static void on_packet(hexabus::sm::Machine& machine,
		const hexabus::Packet& packet,
		const boost::asio::ip::udp::endpoint& remote)
{
	OnPacketVisitor(machine, remote).run(packet);
}

static void on_periodic(hexabus::sm::Machine& machine, boost::asio::deadline_timer& timer)
{
	std::cout << "Running state machine for periodic check: "
		<< machine.run_sm(NULL, 0, NULL)
		<< std::endl;
	timer.expires_from_now(boost::posix_time::seconds(1));
	timer.async_wait(boost::bind(on_periodic, boost::ref(machine), boost::ref(timer)));
}

static uint8_t on_write(uint32_t eid, hexabus::sm::Machine::write_value_t value)
{
	const char* type = NULL;

	switch (value.which()) {
	case 0: type = "bool"; break;
	case 1:
		type = "uint8";
		value = (uint32_t) boost::get<uint8_t>(value);
		break;
	case 2: type = "uint16"; break;
	case 3: type = "uint32"; break;
	case 4: type = "uint64"; break;
	case 5:
		type = "int8";
		value = (int32_t) boost::get<int8_t>(value);
		break;
	case 6: type = "int16"; break;
	case 7: type = "int32"; break;
	case 8: type = "int64"; break;
	case 9: type = "float"; break;
	}

	std::cout << "WRITE\n"
		<< "	EP " << eid << "\n"
		<< "	Type " << type << "\n"
		<< "	Value " << value << std::endl;

	return 0;
}

int main(int argc, char* argv[])
{
	using namespace hexabus::sm;

	if (argc < 3) {
		std::cout << "Usage: hexasm <iface> <sm-file>\n";
		return 1;
	}

	std::string iface(argv[1]);
	std::ifstream file(argv[2], std::ios::binary | std::ios::ate);

	if (file.fail()) {
		std::cerr << "can't open file " << argv[2] << "\n";
		return 1;
	}

	std::vector<uint8_t> machineCode;

	machineCode.resize(std::streamoff(file.tellg()));
	file.seekg(0, std::ios::beg);
	file.read((char*) &machineCode[0], machineCode.size());
	if (file.fail()) {
		std::cerr << "can't read file " << argv[2] << "\n";
		return 1;
	}

	Machine machine(machineCode);
	machine.onWrite(on_write);

	boost::asio::io_service io;
	hexabus::Listener listener(io);
	boost::asio::deadline_timer timer(io);

	listener.listen(iface);
	listener.onPacketReceived(boost::bind(::on_packet, boost::ref(machine), _1, _2));

	::on_periodic(machine, timer);

	io.run();

	return 0;
}
