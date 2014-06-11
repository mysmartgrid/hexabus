#ifndef EEPROM_HPP_
#define EEPROM_HPP_

#include <string>
#include <vector>
#include <map>

#include <boost/array.hpp>
#include <boost/optional.hpp>

#include <stdint.h>

class Eeprom {
	private:
		int fd;

		enum {
			SLAVE_ADDR = 0x50,
			BLOCK_SIZE = 64,
			BLOCK_DATA_SIZE = BLOCK_SIZE - 4,
			CAPACITY = (128 * 1024) / 8,
			BLOCK_COUNT = CAPACITY / BLOCK_SIZE,
		};

		struct eeprom_block {
			uint16_t version;
			uint8_t data[BLOCK_DATA_SIZE];
			uint16_t crc;
		} __attribute__((packed));

		struct stream_header {
			uint32_t length;
		} __attribute__((packed));

		struct stream_footer {
			uint32_t crc;
		} __attribute__((packed));

		struct stream {
			uint16_t version;
			uint16_t first_block;
			uint16_t last_block;
			std::vector<uint8_t> data;
		};

		typedef std::map<uint16_t, stream> versions_t;

		void read_block(uint16_t block, eeprom_block& dest);
		void write_block(uint16_t block, const eeprom_block& src);

		std::vector<eeprom_block>& read_all_blocks(bool force = false);

		std::map<uint16_t, stream> read_valid_versions();
		boost::optional<stream> read_latest_version();

	private:
		std::vector<eeprom_block> _blocks;

	public:
		Eeprom(const std::string& file);

		virtual ~Eeprom();

		boost::optional<std::vector<uint8_t> > read_stream();
		void write_stream(const std::vector<uint8_t>& data);

		void dump_contents();
};

#endif
