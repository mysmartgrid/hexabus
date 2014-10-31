#include <iostream>
#include <utility>
#include <vector>

#include <string.h>

#include "libhexabus/sm/machine.hpp"
#include "libhexabus/socket.hpp"

static void on_packet(hexabus::sm::Machine& machine,
		const hexabus::Packet& packet,
		const boost::asio::ip::udp::endpoint& remote)
{
	using namespace hexabus;
	using namespace hexabus::sm;

	boost::asio::ip::address_v6::bytes_type addr;
	uint32_t eid;
	hxb_sm_value_t value;

	addr = remote.address().to_v6().to_bytes();
	if (const EIDPacket* p = dynamic_cast<const EIDPacket*>(&packet)) {
		eid = p->eid();
	} else {
		return;
	}

	if (const InfoPacket<bool>* p = dynamic_cast<const InfoPacket<bool>*>(&packet)) {
		value.type = p->datatype();
		value.v_uint = p->value();
	} else if (const InfoPacket<uint8_t>* p = dynamic_cast<const InfoPacket<uint8_t>*>(&packet)) {
		value.type = p->datatype();
		value.v_uint = p->value();
	} else if (const InfoPacket<uint32_t>* p = dynamic_cast<const InfoPacket<uint32_t>*>(&packet)) {
		value.type = p->datatype();
		value.v_uint = p->value();
	} else if (const InfoPacket<float>* p = dynamic_cast<const InfoPacket<float>*>(&packet)) {
		value.type = p->datatype();
		value.v_float = p->value();
	} else {
		std::cout << "Received non-sm packet" << std::endl;
		return;
	}

	std::cout << "Running state machine for packet: "
		<< machine.run_sm((const char*) &addr[0], eid, &value)
		<< std::endl;
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
