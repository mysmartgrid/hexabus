#ifndef BINARY_FORMATTER_HPP_
#define BINARY_FORMATTER_HPP_

#include <vector>

#include <stdint.h>

class BinaryFormatter {
	private:
		std::vector<uint8_t>& data;
		std::size_t head;

	public:
		BinaryFormatter(std::vector<uint8_t>& data)
			: data(data), head(0)
		{}

		void get_raw(void* dest, std::size_t len);
		void put_raw(const void* src, std::size_t len);

		uint8_t get_u8();
		uint16_t get_u16();
		uint32_t get_u32();
		uint64_t get_u64();

		void put_u8(uint8_t u);
		void put_u16(uint16_t u);
		void put_u32(uint32_t u);
		void put_u64(uint64_t u);
};

#endif
