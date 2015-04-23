#define BOOST_TEST_MODULE packet_test
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <iomanip>
#include <libhexabus/packet.hpp>
#include <libhexabus/private/serialization.hpp>
#include "testconfig.h"

#define PACK_HEADER 'H', 'X', '0', 'D'
#define PACK_16B 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
#define PACK_65B 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, \
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, \
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, \
	48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64
#define PACK_STR_B 't', 'e', 's', 't', 's', 't', 'r', 'i', 'n', 'g', 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define PACK_STR_S "teststring"
#define PACK_ADDR_B PACK_16B
#define PACK_ADDR_S boost::asio::ip::address_v6({{ PACK_16B }})
#define PACK_U8(u) (u)
#define PACK_U16(u) (((u) >> 8) & 0xFF), ((u) & 0xFF)
#define PACK_U32(u) PACK_U16((u) >> 16), PACK_U16((u) & 0xFFFF)
#define PACK_U64(u) PACK_U32((u) >> 32), PACK_U32((u) & 0xFFFFFFFF)


static std::vector<char> serialize(const hexabus::Packet& p)
{
	return hexabus::serialize(p, p.sequenceNumber(), p.flags());
}

template<typename P>
static P deserialize(const std::vector<unsigned char>& data)
{
	auto deser = hexabus::deserialize(&data[0], data.size());
	return *(P*) deser.get();
}

static bool compare(const std::vector<unsigned char>& expect, const std::vector<char>& in)
{
	if (expect.size() == in.size()) {
		bool eq = true;
		for (unsigned i = 0; i < expect.size(); i++)
			eq &= expect[i] == (unsigned char) in[i];
		if (eq)
			return true;
	}

	std::cout << "Expected:";
	for (char c : expect)
		std::cout << " " << std::hex << std::setfill('0') << std::setw(2) << int((unsigned char) c);
	std::cout << "\n";
	std::cout << "Got:     ";
	for (char c : in)
		std::cout << " " << std::hex << std::setfill('0') << std::setw(2) << int((unsigned char) c);
	std::cout << "\n";

	return false;
}

BOOST_AUTO_TEST_CASE(check_error_packet)
{
	std::cout << "Checking Error out\n";

	hexabus::ErrorPacket packet(17, 0x0102, 0x40, 0x2030);
	std::vector<unsigned char> bytes{{
		PACK_HEADER,
		hexabus::HXB_PTYPE_ERROR, 0x40, PACK_U16(0x2030),
		17, PACK_U16(0x0102),
	}};

	std::vector<char> ser = serialize(packet);

	BOOST_CHECK(compare(bytes, ser));

	std::cout << "Checking Error in\n";

	auto deser = deserialize<hexabus::ErrorPacket>(bytes);
	BOOST_CHECK(packet.type() == deser.type());
	BOOST_CHECK(packet.flags() == deser.flags());
	BOOST_CHECK(packet.sequenceNumber() == deser.sequenceNumber());
	BOOST_CHECK(packet.cause() == deser.cause());
	BOOST_CHECK(packet.code() == deser.code());
}

BOOST_AUTO_TEST_CASE(check_ack_packet)
{
	std::cout << "Checking Ack out\n";

	hexabus::AckPacket packet(0x0102, 0x40, 0x2030);
	std::vector<unsigned char> bytes{{
		PACK_HEADER,
		hexabus::HXB_PTYPE_ACK, 0x40, PACK_U16(0x2030),
		PACK_U16(0x0102),
	}};

	std::vector<char> ser = serialize(packet);

	BOOST_CHECK(compare(bytes, ser));

	std::cout << "Checking Ack in\n";

	auto deser = deserialize<hexabus::AckPacket>(bytes);
	BOOST_CHECK(packet.type() == deser.type());
	BOOST_CHECK(packet.flags() == deser.flags());
	BOOST_CHECK(packet.sequenceNumber() == deser.sequenceNumber());
	BOOST_CHECK(packet.cause() == deser.cause());
}

BOOST_AUTO_TEST_CASE(check_query_packet)
{
	std::cout << "Checking Query out\n";

	hexabus::QueryPacket packet(0x01020304, 0x40, 0x2030);
	std::vector<unsigned char> bytes{{
		PACK_HEADER,
		hexabus::HXB_PTYPE_QUERY, 0x40, PACK_U16(0x2030),
		PACK_U32(0x01020304),
	}};

	std::vector<char> ser = serialize(packet);

	BOOST_CHECK(compare(bytes, ser));

	std::cout << "Checking Query in\n";

	auto deser = deserialize<hexabus::QueryPacket>(bytes);
	BOOST_CHECK(packet.type() == deser.type());
	BOOST_CHECK(packet.flags() == deser.flags());
	BOOST_CHECK(packet.sequenceNumber() == deser.sequenceNumber());
	BOOST_CHECK(packet.eid() == deser.eid());
}

BOOST_AUTO_TEST_CASE(check_epquery_packet)
{
	std::cout << "Checking EPQuery out\n";

	hexabus::EndpointQueryPacket packet(0x01020304, 0x40, 0x2030);
	std::vector<unsigned char> bytes{{
		PACK_HEADER,
		hexabus::HXB_PTYPE_EPQUERY, 0x40, PACK_U16(0x2030),
		PACK_U32(0x01020304),
	}};

	std::vector<char> ser = serialize(packet);

	BOOST_CHECK(compare(bytes, ser));

	std::cout << "Checking EPQuery in\n";

	auto deser = deserialize<hexabus::QueryPacket>(bytes);
	BOOST_CHECK(packet.type() == deser.type());
	BOOST_CHECK(packet.flags() == deser.flags());
	BOOST_CHECK(packet.sequenceNumber() == deser.sequenceNumber());
	BOOST_CHECK(packet.eid() == deser.eid());
}

BOOST_AUTO_TEST_CASE(check_propquery_packet)
{
	std::cout << "Checking PropQuery out\n";

	hexabus::PropertyQueryPacket packet(0x90807060, 0x01020304, 0x40, 0x2030);
	std::vector<unsigned char> bytes{{
		PACK_HEADER,
		hexabus::HXB_PTYPE_EP_PROP_QUERY, 0x40, PACK_U16(0x2030),
		PACK_U32(0x01020304), PACK_U32(0x90807060),
	}};

	std::vector<char> ser = serialize(packet);

	BOOST_CHECK(compare(bytes, ser));

	std::cout << "Checking PropQuery in\n";

	auto deser = deserialize<hexabus::PropertyQueryPacket>(bytes);
	BOOST_CHECK(packet.type() == deser.type());
	BOOST_CHECK(packet.flags() == deser.flags());
	BOOST_CHECK(packet.sequenceNumber() == deser.sequenceNumber());
	BOOST_CHECK(packet.eid() == deser.eid());
}

BOOST_AUTO_TEST_CASE(check_epinfo_packet)
{
	std::cout << "Checking EPInfo out\n";

	hexabus::EndpointInfoPacket packet(0x01020304, 0x24, PACK_STR_S, 0x17, 0x0102);
	std::vector<unsigned char> bytes{{
		PACK_HEADER,
		hexabus::HXB_PTYPE_EPINFO, 0x17, PACK_U16(0x0102),
		PACK_U32(0x01020304), 0x24, PACK_STR_B,
	}};

	std::vector<char> ser = serialize(packet);
	BOOST_CHECK(compare(bytes, ser));

	std::cout << "Checking EPInfo in\n";

	auto deser = deserialize<hexabus::EndpointInfoPacket>(bytes);
	BOOST_CHECK(packet.type() == deser.type());
	BOOST_CHECK(packet.flags() == deser.flags());
	BOOST_CHECK(packet.sequenceNumber() == deser.sequenceNumber());
	BOOST_CHECK(packet.eid() == deser.eid());
	BOOST_CHECK(packet.value() == deser.value());
}

BOOST_AUTO_TEST_CASE(check_epreport_packet)
{
	std::cout << "Checking EPReport out\n";

	hexabus::EndpointReportPacket packet(0x9080, 0x01020304, 0x24, PACK_STR_S, 0x17, 0x0102);
	std::vector<unsigned char> bytes{{
		PACK_HEADER,
		hexabus::HXB_PTYPE_EPREPORT, 0x17, PACK_U16(0x0102),
		PACK_U32(0x01020304), 0x24, PACK_STR_B,
		PACK_U16(0x9080),
	}};

	std::vector<char> ser = serialize(packet);
	BOOST_CHECK(compare(bytes, ser));

	std::cout << "Checking EPReport in\n";

	auto deser = deserialize<hexabus::EndpointReportPacket>(bytes);
	BOOST_CHECK(packet.type() == deser.type());
	BOOST_CHECK(packet.flags() == deser.flags());
	BOOST_CHECK(packet.sequenceNumber() == deser.sequenceNumber());
	BOOST_CHECK(packet.eid() == deser.eid());
	BOOST_CHECK(packet.value() == deser.value());
	BOOST_CHECK(packet.cause() == deser.cause());
}

template<typename T>
struct hxb_dtype;

template<> struct hxb_dtype<bool> : std::integral_constant<int, hexabus::HXB_DTYPE_BOOL> {};
template<> struct hxb_dtype<uint8_t> : std::integral_constant<int, hexabus::HXB_DTYPE_UINT8> {};
template<> struct hxb_dtype<uint16_t> : std::integral_constant<int, hexabus::HXB_DTYPE_UINT16> {};
template<> struct hxb_dtype<uint32_t> : std::integral_constant<int, hexabus::HXB_DTYPE_UINT32> {};
template<> struct hxb_dtype<uint64_t> : std::integral_constant<int, hexabus::HXB_DTYPE_UINT64> {};
template<> struct hxb_dtype<int8_t> : std::integral_constant<int, hexabus::HXB_DTYPE_SINT8> {};
template<> struct hxb_dtype<int16_t> : std::integral_constant<int, hexabus::HXB_DTYPE_SINT16> {};
template<> struct hxb_dtype<int32_t> : std::integral_constant<int, hexabus::HXB_DTYPE_SINT32> {};
template<> struct hxb_dtype<int64_t> : std::integral_constant<int, hexabus::HXB_DTYPE_SINT64> {};
template<> struct hxb_dtype<float> : std::integral_constant<int, hexabus::HXB_DTYPE_FLOAT> {};
template<> struct hxb_dtype<std::array<uint8_t, 16>> : std::integral_constant<int, hexabus::HXB_DTYPE_16BYTES> {};
template<> struct hxb_dtype<std::array<uint8_t, 65>> : std::integral_constant<int, hexabus::HXB_DTYPE_65BYTES> {};
template<> struct hxb_dtype<std::string> : std::integral_constant<int, hexabus::HXB_DTYPE_128STRING> {};

typedef std::array<uint8_t, 16> array_16;
typedef std::array<uint8_t, 65> array_65;

#define TEST_ALL() \
	do { \
		TEST_ONE_SPEC(bool, true, 1); \
		TEST_ONE_SPEC(uint8_t, 0x40, 0x40); \
		TEST_ONE_SPEC(uint16_t, 0x4050, PACK_U16(0x4050)); \
		TEST_ONE_SPEC(uint32_t, 0x40506070, PACK_U32(0x40506070)); \
		TEST_ONE_SPEC(uint64_t, 0x4050607001020304, PACK_U64(0x4050607001020304)); \
		TEST_ONE_SPEC(int8_t, -0x40, (unsigned char) -0x40); \
		TEST_ONE_SPEC(int16_t, -0x4050, PACK_U16(-0x4050)); \
		TEST_ONE_SPEC(int32_t, -0x40506070, PACK_U32(-0x40506070)); \
		TEST_ONE_SPEC(int64_t, -0x4050607001020304, PACK_U64(-0x4050607001020304)); \
		TEST_ONE_SPEC(float, 23.42, PACK_U32(0x41BB5C29)); \
		TEST_ONE_SPEC(array_16, {{PACK_16B}}, PACK_16B); \
		TEST_ONE_SPEC(array_65, {{PACK_65B}}, PACK_65B); \
		TEST_ONE_SPEC(std::string, PACK_STR_S, PACK_STR_B); \
	} while (0)

#define CHECK_COMMON_FIELDS(packet, deser) \
	do { \
		BOOST_CHECK(packet.type() == deser.type()); \
		BOOST_CHECK(packet.flags() == deser.flags()); \
		BOOST_CHECK(packet.sequenceNumber() == deser.sequenceNumber()); \
	} while (0)

#define CHECK_EID_COMMON_FIELDS(packet, deser) \
	do { \
		CHECK_COMMON_FIELDS(packet, deser); \
		BOOST_CHECK(packet.eid() == deser.eid()); \
		BOOST_CHECK(packet.value() == deser.value()); \
	} while (0)

BOOST_AUTO_TEST_CASE(check_info_packet)
{
#define TEST_ONE_SPEC(TYPE, VALUE, ...) \
	do { \
		std::cout << "Checking Info<" #TYPE "> out\n"; \
		\
		hexabus::InfoPacket<TYPE> packet(0x01020304, VALUE, 0x17, 0x0102); \
		std::vector<unsigned char> bytes{{ \
			PACK_HEADER, \
			hexabus::HXB_PTYPE_INFO, 0x17, PACK_U16(0x0102), \
			PACK_U32(0x01020304), hxb_dtype<TYPE>::value, __VA_ARGS__, \
		}}; \
		\
		std::vector<char> ser = serialize(packet); \
		BOOST_CHECK(compare(bytes, ser)); \
		\
		std::cout << "Checking Info<" #TYPE "> in\n"; \
		\
		auto deser = deserialize<hexabus::InfoPacket<TYPE>>(bytes); \
		CHECK_EID_COMMON_FIELDS(packet, deser); \
	} while (0)

	TEST_ALL();

#undef TEST_ONE_SPEC
}

BOOST_AUTO_TEST_CASE(check_report_packet)
{
#define TEST_ONE_SPEC(TYPE, VALUE, ...) \
	do { \
		std::cout << "Checking Report<" #TYPE "> out\n"; \
		\
		hexabus::ReportPacket<TYPE> packet(0x0102, 0x01020304, VALUE, 0x17, 0x0102); \
		std::vector<unsigned char> bytes{{ \
			PACK_HEADER, \
			hexabus::HXB_PTYPE_REPORT, 0x17, PACK_U16(0x0102), \
			PACK_U32(0x01020304), hxb_dtype<TYPE>::value, __VA_ARGS__, \
			PACK_U16(0x0102), \
		}}; \
		\
		std::vector<char> ser = serialize(packet); \
		BOOST_CHECK(compare(bytes, ser)); \
		\
		std::cout << "Checking Report<" #TYPE "> in\n"; \
		\
		auto deser = deserialize<hexabus::ReportPacket<TYPE>>(bytes); \
		CHECK_EID_COMMON_FIELDS(packet, deser); \
		BOOST_CHECK(packet.cause() == deser.cause()); \
	} while (0)

	TEST_ALL();

#undef TEST_ONE_SPEC
}

BOOST_AUTO_TEST_CASE(check_proxyinfo_packet)
{
#define TEST_ONE_SPEC(TYPE, VALUE, ...) \
	do { \
		std::cout << "Checking ProxyInfo<" #TYPE "> out\n"; \
		\
		hexabus::ProxyInfoPacket<TYPE> packet(PACK_ADDR_S, 0x01020304, VALUE, 0x17, 0x0102); \
		std::vector<unsigned char> bytes{{ \
			PACK_HEADER, \
			hexabus::HXB_PTYPE_PINFO, 0x17, PACK_U16(0x0102), \
			PACK_U32(0x01020304), hxb_dtype<TYPE>::value, __VA_ARGS__, \
			PACK_ADDR_B, \
		}}; \
		\
		std::vector<char> ser = serialize(packet); \
		BOOST_CHECK(compare(bytes, ser)); \
		\
		std::cout << "Checking ProxyInfo<" #TYPE "> in\n"; \
		\
		auto deser = deserialize<hexabus::ProxyInfoPacket<TYPE>>(bytes); \
		CHECK_EID_COMMON_FIELDS(packet, deser); \
		BOOST_CHECK(packet.origin() == deser.origin()); \
	} while (0)

	TEST_ALL();

#undef TEST_ONE_SPEC
}

BOOST_AUTO_TEST_CASE(check_write_packet)
{
#define TEST_ONE_SPEC(TYPE, VALUE, ...) \
	do { \
		std::cout << "Checking Write<" #TYPE "> out\n"; \
		\
		hexabus::WritePacket<TYPE> packet(0x01020304, VALUE, 0x17, 0x0102); \
		std::vector<unsigned char> bytes{{ \
			PACK_HEADER, \
			hexabus::HXB_PTYPE_WRITE, 0x17, PACK_U16(0x0102), \
			PACK_U32(0x01020304), hxb_dtype<TYPE>::value, __VA_ARGS__, \
		}}; \
		\
		std::vector<char> ser = serialize(packet); \
		BOOST_CHECK(compare(bytes, ser)); \
		\
		std::cout << "Checking Write<" #TYPE "> in\n"; \
		\
		auto deser = deserialize<hexabus::WritePacket<TYPE>>(bytes); \
		CHECK_EID_COMMON_FIELDS(packet, deser); \
	} while (0)

	TEST_ALL();

#undef TEST_ONE_SPEC
}

BOOST_AUTO_TEST_CASE(check_propwrite_packet)
{
#define TEST_ONE_SPEC(TYPE, VALUE, ...) \
	do { \
		std::cout << "Checking PropWrite<" #TYPE "> out\n"; \
		\
		hexabus::PropertyWritePacket<TYPE> packet(0x90807060, 0x01020304, VALUE, 0x17, 0x0102); \
		std::vector<unsigned char> bytes{{ \
			PACK_HEADER, \
			hexabus::HXB_PTYPE_EP_PROP_WRITE, 0x17, PACK_U16(0x0102), \
			PACK_U32(0x01020304), hxb_dtype<TYPE>::value, PACK_U32(0x90807060), __VA_ARGS__, \
		}}; \
		\
		std::vector<char> ser = serialize(packet); \
		BOOST_CHECK(compare(bytes, ser)); \
		\
		std::cout << "Checking PropWrite<" #TYPE "> in\n"; \
		\
		auto deser = deserialize<hexabus::PropertyWritePacket<TYPE>>(bytes); \
		CHECK_EID_COMMON_FIELDS(packet, deser); \
		BOOST_CHECK(packet.propid() == deser.propid()); \
	} while (0)

	TEST_ALL();

#undef TEST_ONE_SPEC
}

BOOST_AUTO_TEST_CASE(check_propreport_packet)
{
#define TEST_ONE_SPEC(TYPE, VALUE, ...) \
	do { \
		std::cout << "Checking PropReport<" #TYPE "> out\n"; \
		\
		hexabus::PropertyReportPacket<TYPE> packet(0x90807060, 0x0102, 0x01020304, VALUE, 0x17, 0x0102); \
		std::vector<unsigned char> bytes{{ \
			PACK_HEADER, \
			hexabus::HXB_PTYPE_EP_PROP_REPORT, 0x17, PACK_U16(0x0102), \
			PACK_U32(0x01020304), hxb_dtype<TYPE>::value, PACK_U32(0x90807060), __VA_ARGS__, \
			PACK_U16(0x0102), \
		}}; \
		\
		std::vector<char> ser = serialize(packet); \
		BOOST_CHECK(compare(bytes, ser)); \
		\
		std::cout << "Checking PropReport<" #TYPE "> in\n"; \
		\
		auto deser = deserialize<hexabus::PropertyReportPacket<TYPE>>(bytes); \
		CHECK_EID_COMMON_FIELDS(packet, deser); \
		BOOST_CHECK(packet.cause() == deser.cause()); \
		BOOST_CHECK(packet.nextid() == deser.nextid()); \
	} while (0)

	TEST_ALL();

#undef TEST_ONE_SPEC
}
