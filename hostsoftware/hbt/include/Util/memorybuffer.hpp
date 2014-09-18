#ifndef INCLUDE_UTIL_MEMORYBUFFER_HPP_2B2C6567CF10D7F0
#define INCLUDE_UTIL_MEMORYBUFFER_HPP_2B2C6567CF10D7F0

#include <cstring>
#include <string>
#include <vector>

#include "Util/range.hpp"

namespace hbt {
namespace util {

class MemoryBuffer {
	private:
		std::vector<char> _data;

		template<typename T>
		static void checkIteratorValidity()
		{
			static_assert(std::is_same<T, char>::value || std::is_same<T, uint8_t>::value,
					"only char types allowed");
		}

	public:
		typedef decltype(_data.begin()) iterator;
		typedef decltype(_data.cbegin()) const_iterator;

	public:
		MemoryBuffer() = default;

		explicit MemoryBuffer(const std::vector<char>& data)
			: _data(data)
		{}

		explicit MemoryBuffer(const std::string& data)
			: _data(data.begin(), data.end())
		{}

		explicit MemoryBuffer(const std::vector<uint8_t>& data)
			: _data()
		{
			_data.resize(data.size());
			memcpy(_data.data(), data.data(), data.size());
		}

		explicit MemoryBuffer(std::vector<char>&& data)
			: _data(std::forward<std::vector<char>>(data))
		{}

		static MemoryBuffer loadFile(const std::string& path);
		void writeFile(const std::string& path);

		iterator begin() { return _data.begin(); }
		iterator end() { return _data.end(); }

		const_iterator cbegin() const { return _data.cbegin(); }
		const_iterator cend() const { return _data.cend(); }

		void insert(iterator pos, const void* data, size_t size)
		{
			const char* first = reinterpret_cast<const char*>(data);
			_data.insert(pos, first, first + size);
		}

		void replace(iterator pos, const void* data, size_t size)
		{
			auto begin = std::distance(_data.begin(), pos);

			(void) _data.at(begin);
			(void) _data.at(begin + size - 1);

			memcpy(_data.data() + begin, data, size);
		}

		void append(const void* data, size_t size)
		{
			insert(end(), data, size);
		}

		void reserve(size_t size)
		{
			_data.reserve(size);
		}

		size_t size() const
		{
			return _data.size();
		}

		template<typename T>
		hbt::util::range<T*> range()
		{
			checkIteratorValidity<T>();

			char* begin = &_data[0];
			char* end = begin + _data.size();
			return { reinterpret_cast<T*>(begin), reinterpret_cast<T*>(end) };
		}

		template<typename T>
		hbt::util::range<const T*> range() const
		{
			checkIteratorValidity<T>();

			const char* begin = &_data[0];
			const char* end = begin + _data.size();
			return { reinterpret_cast<const T*>(begin), reinterpret_cast<const T*>(end) };
		}

		template<typename T>
		hbt::util::range<const T*> crange() const
		{
			return range<T>();
		}
};

}
}

#endif
