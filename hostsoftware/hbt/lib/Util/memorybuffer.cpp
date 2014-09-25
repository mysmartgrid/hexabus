#include "Util/memorybuffer.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

static bool readStream(std::istream& stream, std::vector<char>& result)
{
	std::vector<char> buffer(65535);

	while (stream && !stream.eof()) {
		result.reserve(result.size() + buffer.size());

		stream.read(&buffer[0], buffer.size());
		int read = stream.gcount();

		if (!stream.bad())
			result.insert(result.end(), buffer.begin(), buffer.begin() + read);
	}

	result.shrink_to_fit();

	return stream.eof();
}

static bool writeStream(std::ostream& stream, const std::vector<char>& contents)
{
	stream.write(&contents[0], contents.size());

	return !stream.bad();
}

namespace hbt {
namespace util {

static std::unique_ptr<MemoryBuffer> tryLoadFile(const std::string& path)
{
	std::vector<char> result;
	bool success;

	std::ifstream file(path);

	if (!file)
		return {nullptr};

	if (!readStream(file, result))
		return {nullptr};

	return std::unique_ptr<MemoryBuffer>(new MemoryBuffer(std::move(result)));
}

MemoryBuffer MemoryBuffer::loadFile(const std::string& path, bool allowStdin)
{
	std::vector<char> result;
	bool success;

	if (path == "-" && allowStdin) {
		success = readStream(std::cin, result);
	} else {
		std::ifstream file(path);

		if (!file)
			throw std::runtime_error("could not open file " + path);

		success = readStream(file, result);
	}

	if (!success)
		throw std::runtime_error("can't read file " + path);

	return MemoryBuffer(std::move(result));
}

void MemoryBuffer::writeFile(const std::string& path, bool allowStdout)
{
	bool success;

	if (path == "-" && allowStdout) {
		success = writeStream(std::cout, _data);
	} else {
		std::ofstream file(path, std::ios_base::trunc);

		if (!file)
			throw std::runtime_error("could not open file " + path);

		success = writeStream(file, _data);
	}

	if (!success)
		throw std::runtime_error("can't write file " + path);
}

}
}
