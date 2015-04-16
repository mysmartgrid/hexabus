#define BOOST_TEST_MODULE packet_test
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <libhexabus/packet.hpp>
#include <libhexabus/private/serialization.hpp>
#include "testconfig.h"


/**
 * see http://www.boost.org/doc/libs/1_43_0/libs/test/doc/html/tutorials/hello-the-testing-world.html
 */

BOOST_AUTO_TEST_CASE ( check_sanity ) {
	try {
		std::cout << "Demo test case: Checking world sanity." << std::endl;
		BOOST_CHECK_EQUAL (42, 42);
		BOOST_CHECK( 23 != 42 );        // #1 continues on error
		BOOST_REQUIRE( 23 != 42 );      // #2 throws on error

	} catch (std::exception const & ex) {
		BOOST_ERROR ( ex.what() );
	}
	if( 23 == 42 ) {
		BOOST_FAIL( "23 == 42, oh noes");             // #4 throws on error
	}
}

BOOST_AUTO_TEST_CASE ( check_write_uint8_packet_generation ) {
	std::cout << "Checking generation of uint8 write packet against stored reference packet." << std::endl;
	unsigned char testpacket[] = { 'H', 'X', '0', 'C', // Header
		0x04,               // Packet Type: Write
		0x00,               // Flags: None
		0, 0, 0, 23,        // Endpoint ID: 23
		0x01,               // Datatype: Uint8
		42,                 // Value: 42
		0x9b, 0x2b          // CRC
	};

	hexabus::WritePacket<uint8_t> p(23, 42, 0);
	std::vector<char> pi8 = hexabus::serialize(p);

	if (sizeof(testpacket) != pi8.size())
	  BOOST_FAIL("Size of generated packet differs from test packet");

	bool fail = false;
	for(size_t i = 0; i < pi8.size(); i++) {
		std::cout << "Byte " << std::dec << i << " Generated: 0x" << std::hex << (short int)(unsigned char)pi8[i] << "\t" << " Reference: 0x" << (short int)testpacket[i] << std::endl;
		if((unsigned char)pi8[i] != testpacket[i])
			fail = true;
	}

	if(fail)
		BOOST_FAIL("Generated packet differs from reference packet.");
}

BOOST_AUTO_TEST_CASE ( check_write_bool_packet_generation ) {
	std::cout << "Checking generation of bool write packet against stored reference packet." << std::endl;
	unsigned char testpacket[] = { 'H', 'X', '0', 'C', // Header
		0x04,               // Packet Type: Write
		0x00,               // Flags: None
		0, 0, 0, 23,        // Endpoint ID: 23
		0x00,               // Datatype: Boolean
		0x01,               // Value: true
		0x1d, 0x22          // CRC
	};

	hexabus::WritePacket<bool> p(23, true, 0);
	std::vector<char> pi8 = hexabus::serialize(p);

	if (sizeof(testpacket) != pi8.size())
		BOOST_FAIL("Size of generated packet differs from test packet");

	bool fail = false;
	for(size_t i = 0; i < pi8.size(); i++) {
		std::cout << "Byte " << std::dec << i << " Generated: 0x" << std::hex << (short int)(unsigned char)pi8[i] << "\t" << " Reference: 0x" << (short int)testpacket[i] << std::endl;
		if((unsigned char)pi8[i] != testpacket[i])
			fail = true;
	}

	if(fail)
		BOOST_FAIL("Generated packet differs from reference packet.");
}

BOOST_AUTO_TEST_CASE ( check_write_uint32_packet_generation ) {
	std::cout << "Checking generation of uint32 write packet against stored reference packet." << std::endl;
	unsigned char testpacket[] = { 'H', 'X', '0', 'C', // Header
		0x04,               // Packet Type: Write
		0x00,               // Flags: None
		0, 0, 0, 42,        // Endpoint ID: 42
		0x03,               // Datatype: Uint32
		0xfc, 0xde, 0x41, 0xb2, // Value: 4242424242
		0xd6, 0x3e          // CRC
	};

	hexabus::WritePacket<uint32_t> p(42, 4242424242u, 0);
	std::vector<char> pi32 = hexabus::serialize(p);

	if (sizeof(testpacket) != pi32.size())
		BOOST_FAIL("Size of generated packet differs from test packet");

	bool fail = false;
	for(size_t i = 0; i < pi32.size(); i++) {
		std::cout << "Byte " << std::dec << i << " Generated: 0x" << std::hex << (short int)(unsigned char)pi32[i] << "\t" << " Reference: 0x" << (short int)testpacket[i] << std::endl;
		if((unsigned char)pi32[i] != testpacket[i])
			fail = true;
	}

	if(fail)
		BOOST_FAIL("Generated packet differs from reference packet.");
}

BOOST_AUTO_TEST_CASE ( check_write_float_packet_generation ) {
	std::cout << "Checking generation of float write packet against stored reference packet." << std::endl;
	unsigned char testpacket[] = { 'H', 'X', '0', 'C', // Header
		0x04,               // Packet Type: Write
		0x00,               // Flags: None
		0, 0, 0, 42,        // Endpoint ID: 42
		0x09,               // Datatype: Uint32
		0x41, 0xbb, 0x5c, 0x29, // Value 23.42
		0x43, 0xc6          // CRC
	};

	hexabus::WritePacket<float> p(42, 23.42, 0);
	std::vector<char> pif = hexabus::serialize(p);

	if (sizeof(testpacket) != pif.size())
		BOOST_FAIL("Size of generated packet differs from test packet");

	bool fail = false;
	for(size_t i = 0; i < pif.size(); i++) {
		std::cout << "Byte " << std::dec << i << " Generated: 0x" << std::hex << (short int)(unsigned char)pif[i] << "\t" << " Reference: 0x" << (short int)testpacket[i] << std::endl;
		if((unsigned char)pif[i] != testpacket[i])
			fail = true;
	}

	if(fail)
		BOOST_FAIL("Generated packet differs from reference packet.");
}
//BOOST_AUTO_TEST_SUITE_END()
