#define BOOST_TEST_MODULE packet_test
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <libhexabus/packet.hpp>
#include <libhexabus/private/serialization.hpp>
#include "testconfig.h"

// {{{ Default values for typed packets

template<typename T>
struct value_default;

template<>
struct value_default<bool> {
	static bool value;
	static boost::array<char, 1> bytes;
	static const char* name;
	enum { datatype = HXB_DTYPE_BOOL };
};
bool value_default<bool>::value = true;
const char* value_default<bool>::name = "bool";
boost::array<char, 1> value_default<bool>::bytes = {{ 1 }};

template<>
struct value_default<uint8_t> {
	static uint8_t value;
	static boost::array<char, 1> bytes;
	static const char* name;
	enum { datatype = HXB_DTYPE_UINT8 };
};
uint8_t value_default<uint8_t>::value = 0x42;
const char* value_default<uint8_t>::name = "uint8";
boost::array<char, 1> value_default<uint8_t>::bytes = {{ 0x42 }};

template<>
struct value_default<uint32_t> {
	static uint32_t value;
	static boost::array<char, 4> bytes;
	static const char* name;
	enum { datatype = HXB_DTYPE_UINT32 };
};
uint32_t value_default<uint32_t>::value = 0x01020304;
const char* value_default<uint32_t>::name = "uint32";
boost::array<char, 4> value_default<uint32_t>::bytes = {{ 1, 2, 3, 4 }};

template<>
struct value_default<float> {
	static float value;
	static boost::array<char, 4> bytes;
	static const char* name;
	enum { datatype = HXB_DTYPE_FLOAT };
};
float value_default<float>::value = 23.42f;
const char* value_default<float>::name = "float";
boost::array<char, 4> value_default<float>::bytes = {{ 0x41, 0xbb, 0x5c, 0x29 }};

template<>
struct value_default<boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> > {
	static boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> value;
	static boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> bytes;
	static const char* name;
	enum { datatype = HXB_DTYPE_16BYTES };
};
boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> value_default<boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> >::value = {{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }};
const char* value_default<boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> >::name = "16 bytes";
boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> value_default<boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> >::bytes =
	value_default<boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH> >::value;

template<>
struct value_default<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> > {
	static boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> value;
	static boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> bytes;
	static const char* name;
	enum { datatype = HXB_DTYPE_65BYTES };
};
boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> value_default<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >::value = {{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66 }};
const char* value_default<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >::name = "65 bytes";
boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> value_default<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >::bytes =
	value_default<boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH> >::value;

template<>
struct value_default<std::string> {
	static std::string value;
	static boost::array<char, HXB_STRING_PACKET_BUFFER_LENGTH + 1> bytes;
	static const char* name;
	enum { datatype = HXB_DTYPE_128STRING };
};
std::string value_default<std::string>::value = "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x7f";
const char* value_default<std::string>::name = "std::string";
boost::array<char, HXB_STRING_PACKET_BUFFER_LENGTH + 1> value_default<std::string>::bytes = {{ 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x12, 0x13, 0x14,
	0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
	0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
	0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
	0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0
}};

// }}}

void generate_header(std::vector<char>& target, uint8_t type, uint8_t flags, uint16_t seqNum)
{
	target.push_back('H');
	target.push_back('X');
	target.push_back('0');
	target.push_back('D');
	target.push_back(type);
	target.push_back(flags);
	target.push_back((seqNum >> 8) & 0xFF);
	target.push_back((seqNum >> 0) & 0xFF);
}

void generate_error(std::vector<char>& vect, hexabus::ErrorPacket& packet)
{
	vect.clear();
	packet = hexabus::ErrorPacket(0x23, 0x0201, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_ERROR, 0xAA, 0xBB98);
	vect.push_back(0x23);
	vect.push_back(0x02);
	vect.push_back(0x01);
}

void generate_ack(std::vector<char>& vect, hexabus::AckPacket& packet)
{
	vect.clear();
	packet = hexabus::AckPacket(0xAA54, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_ACK, 0xAA, 0xBB98);
	vect.push_back(0xAA);
	vect.push_back(0x54);
}

template<typename T>
void generate_info_part(std::vector<char>& vect, uint32_t eid)
{
	vect.push_back((eid >> 24) & 0xFF);
	vect.push_back((eid >> 16) & 0xFF);
	vect.push_back((eid >> 8) & 0xFF);
	vect.push_back((eid >> 0) & 0xFF);
	vect.push_back(value_default<T>::datatype);
	vect.insert(vect.end(), value_default<T>::bytes.begin(), value_default<T>::bytes.end());
}

template<typename T>
void generate_info(std::vector<char>& vect, hexabus::InfoPacket<T>& packet)
{
	vect.clear();
	packet = hexabus::InfoPacket<T>(0x14131210, value_default<T>::value, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_INFO, 0xAA, 0xBB98);
	generate_info_part<T>(vect, 0x14131210);
}

void generate_epinfo(std::vector<char>& vect, hexabus::EndpointInfoPacket& packet)
{
	vect.clear();
	packet = hexabus::EndpointInfoPacket(0x14131210, HXB_DTYPE_128STRING, value_default<std::string>::value, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_EPINFO, 0xAA, 0xBB98);
	generate_info_part<std::string>(vect, 0x14131210);
}

template<typename T>
void generate_pinfo(std::vector<char>& vect, hexabus::ProxyInfoPacket<T>& packet)
{
	boost::asio::ip::address_v6 addr = boost::asio::ip::address_v6::from_string("1:2:3:4:5:6:7:8");
	boost::asio::ip::address_v6::bytes_type bytes = addr.to_bytes();

	vect.clear();
	packet = hexabus::ProxyInfoPacket<T>(addr, 0x14131210, value_default<T>::value, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_PINFO, 0xAA, 0xBB98);
	generate_info_part<T>(vect, 0x14131210);
	vect.insert(vect.end(), bytes.begin(), bytes.end());
}

template<typename T>
void generate_write(std::vector<char>& vect, hexabus::WritePacket<T>& packet)
{
	vect.clear();
	packet = hexabus::WritePacket<T>(0x14131210, value_default<T>::value, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_WRITE, 0xAA, 0xBB98);
	generate_info_part<T>(vect, 0x14131210);
}

template<typename T>
void generate_report(std::vector<char>& vect, hexabus::ReportPacket<T>& packet)
{
	vect.clear();
	packet = hexabus::ReportPacket<T>(0x9876, 0x14131210, value_default<T>::value, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_REPORT, 0xAA, 0xBB98);
	generate_info_part<T>(vect, 0x14131210);
	vect.push_back(0x98);
	vect.push_back(0x76);
}

void generate_epreport(std::vector<char>& vect, hexabus::EndpointReportPacket& packet)
{
	vect.clear();
	packet = hexabus::EndpointReportPacket(0x9876, 0x14131210, HXB_DTYPE_128STRING, value_default<std::string>::value, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_EPREPORT, 0xAA, 0xBB98);
	generate_info_part<std::string>(vect, 0x14131210);
	vect.push_back(0x98);
	vect.push_back(0x76);
}

void generate_query(std::vector<char>& vect, hexabus::QueryPacket& packet)
{
	vect.clear();
	packet = hexabus::QueryPacket(0x14131210, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_QUERY, 0xAA, 0xBB98);
	vect.push_back(0x14);
	vect.push_back(0x13);
	vect.push_back(0x12);
	vect.push_back(0x10);
}

void generate_epquery(std::vector<char>& vect, hexabus::EndpointQueryPacket& packet)
{
	vect.clear();
	packet = hexabus::EndpointQueryPacket(0x14131210, 0xAA, 0xBB98);
	generate_header(vect, HXB_PTYPE_EPQUERY, 0xAA, 0xBB98);
	vect.push_back(0x14);
	vect.push_back(0x13);
	vect.push_back(0x12);
	vect.push_back(0x10);
}

void match_vectors(const std::vector<char>& expected, const std::vector<char>& generated)
{
	if (expected != generated) {
		std::cout << "Expected: ";
		for (size_t i = 0; i < expected.size(); i++) {
			std::cout << std::hex << std::setfill('0') << "0x" << std::setw(2) << (0xFF & uint32_t(expected[i])) << " ";
		}
		std::cout << std::endl;
		std::cout << "Got:      ";
		for (size_t i = 0; i < generated.size(); i++) {
			std::cout << std::hex << std::setfill('0') << "0x" << std::setw(2) << (0xFF & uint32_t(generated[i])) << " ";
		}
		std::cout << std::endl;

		BOOST_FAIL("Generated packet differs from expected packet");
	}
}

void do_actual_test(const std::vector<char>& exp, const hexabus::Packet& p)
{
	std::vector<char> gen = hexabus::serialize(p, p.sequenceNumber());

	match_vectors(exp, gen);
	std::cout << "Serialization works" << std::endl;
	hexabus::Packet::Ptr ptr = hexabus::deserialize(&gen[0], gen.size());
	gen = hexabus::serialize(*ptr, ptr->sequenceNumber());
	match_vectors(exp, gen);
	std::cout << "Roundtripping works" << std::endl;
}

BOOST_AUTO_TEST_CASE(check_error)
{
	std::cout << "Checking ERROR" << std::endl;

	std::vector<char> exp;
	hexabus::ErrorPacket p(0, 0);
	generate_error(exp, p);

	do_actual_test(exp, p);
}

BOOST_AUTO_TEST_CASE(check_ack)
{
	std::cout << "Checking ACK" << std::endl;

	std::vector<char> exp;
	hexabus::AckPacket p(0);
	generate_ack(exp, p);

	do_actual_test(exp, p);
}

BOOST_AUTO_TEST_CASE(check_epinfo)
{
	std::cout << "Checking EPINFO" << std::endl;

	std::vector<char> exp;
	hexabus::EndpointInfoPacket p(0, 0, "");
	generate_epinfo(exp, p);

	do_actual_test(exp, p);
}

typedef boost::mpl::list<
	bool,
	uint8_t,
	uint32_t,
	float,
	boost::array<char, HXB_16BYTES_PACKET_BUFFER_LENGTH>,
	boost::array<char, HXB_65BYTES_PACKET_BUFFER_LENGTH>,
	std::string> value_types;

BOOST_AUTO_TEST_CASE_TEMPLATE(check_info, T, value_types)
{
	std::cout << "Checking INFO<" << value_default<T>::name << ">" << std::endl;

	std::vector<char> exp;
	hexabus::InfoPacket<T> p(0, value_default<T>::value);
	generate_info(exp, p);

	do_actual_test(exp, p);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(check_report, T, value_types)
{
	std::cout << "Checking REPORT<" << value_default<T>::name << ">" << std::endl;

	std::vector<char> exp;
	hexabus::ReportPacket<T> p(0, 0, value_default<T>::value);
	generate_report(exp, p);

	do_actual_test(exp, p);
}

BOOST_AUTO_TEST_CASE(check_epreport)
{
	std::cout << "Checking EPREPORT" << std::endl;

	std::vector<char> exp;
	hexabus::EndpointReportPacket p(0, 0, 0, "");
	generate_epreport(exp, p);

	do_actual_test(exp, p);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(check_write, T, value_types)
{
	std::cout << "Checking WRITE<" << value_default<T>::name << ">" << std::endl;

	std::vector<char> exp;
	hexabus::WritePacket<T> p(0, value_default<T>::value);
	generate_write(exp, p);

	do_actual_test(exp, p);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(check_pinfo, T, value_types)
{
	std::cout << "Checking PINFO<" << value_default<T>::name << ">" << std::endl;

	std::vector<char> exp;
	hexabus::ProxyInfoPacket<T> p(boost::asio::ip::address_v6::any(), 0, value_default<T>::value);
	generate_pinfo(exp, p);

	do_actual_test(exp, p);
}

BOOST_AUTO_TEST_CASE(check_query)
{
	std::cout << "Checking QUERY" << std::endl;

	std::vector<char> exp;
	hexabus::QueryPacket p(0);
	generate_query(exp, p);

	do_actual_test(exp, p);
}

BOOST_AUTO_TEST_CASE(check_epquery)
{
	std::cout << "Checking EPQUERY" << std::endl;

	std::vector<char> exp;
	hexabus::EndpointQueryPacket p(0);
	generate_epquery(exp, p);

	do_actual_test(exp, p);
}

//BOOST_AUTO_TEST_SUITE_END()
