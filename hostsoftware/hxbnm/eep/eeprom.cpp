#include "eeprom.hpp"

#include <map>
#include <algorithm>
#include <stdexcept>

#include <boost/crc.hpp>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>

#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <endian.h>

uint32_t crc_stream(const void* p, size_t len)
{
	boost::crc_32_type engine;

	engine.process_bytes(p, len);
	return engine.checksum();
}

uint16_t crc_block(const void* p, size_t len)
{
	boost::crc_ccitt_type engine;

	engine.process_bytes(p, len);
	return engine.checksum();
}

Eeprom::Eeprom(const std::string& file)
	: fd(open(file.c_str(), O_RDWR))
{
	if (fd < 0)
		throw std::runtime_error(strerror(errno));

	if (ioctl(fd, I2C_SLAVE, SLAVE_ADDR) < 0) {
		close(fd);
		throw std::runtime_error(strerror(errno));
	}

	flock lock;
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	if (fcntl(fd, F_SETLKW, &lock) < 0) {
		close(fd);
		throw std::runtime_error(strerror(errno));
	}
}

Eeprom::~Eeprom()
{
	flock lock;
	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	fcntl(fd, F_SETLK, &lock);

	close(fd);
}

void Eeprom::read_block(uint16_t block, eeprom_block& dest)
{
	uint8_t addr[2] = { (block * BLOCK_SIZE) >> 8, (block * BLOCK_SIZE) & 0xFF };

	if (write(fd, addr, sizeof(addr)) < int(sizeof(addr)))
		throw std::runtime_error(strerror(errno));

	if (read(fd, &dest, BLOCK_SIZE) < BLOCK_SIZE)
		throw std::runtime_error(strerror(errno));
}

void Eeprom::write_block(uint16_t block, const eeprom_block& src)
{
	std::vector<uint8_t> buffer;

	buffer.reserve(BLOCK_SIZE + 2);
	buffer.push_back((block * BLOCK_SIZE) >> 8);
	buffer.push_back((block * BLOCK_SIZE) & 0xFF);
	buffer.resize(buffer.size() + BLOCK_SIZE);
	memcpy(&buffer[2], &src, BLOCK_SIZE);

	int err = write(fd, &buffer[0], buffer.size());
	if (err < 0)
		throw std::runtime_error(strerror(errno));

	while (write(fd, &buffer[0], 0) < 0 && errno == EIO)
		usleep(1000);
}



std::vector<Eeprom::eeprom_block>& Eeprom::read_all_blocks(bool force)
{
	if (!_blocks.size() || force) {
		_blocks.clear();
		_blocks.resize(BLOCK_COUNT);

		for (uint16_t i = 0; i < BLOCK_COUNT; i++) {
			read_block(i, _blocks[i]);
		}
	}

	return _blocks;
}

Eeprom::versions_t Eeprom::read_valid_versions()
{
	versions_t all_versions;

	std::vector<eeprom_block>& blocks = read_all_blocks();
	uint32_t first_offset = 0;
	uint16_t first_version = be16toh(blocks.front().version);

	for (size_t i = 0; i < blocks.size(); i++) {
		const eeprom_block& block = blocks[i];

		if (!crc_block(&block, BLOCK_SIZE)) {
			uint16_t v = be16toh(block.version);
			stream& s = all_versions[v];

			if (!s.data.size()) {
				s.version = v;
				s.first_block = i;
			}

			if (s.version == first_version && i > 0 && i - 1 > s.last_block) {
				if (first_offset == 0) {
					s.first_block = i;
				}
				s.data.insert(s.data.begin() + first_offset,
					block.data,
					block.data + sizeof(block.data));
				first_offset += BLOCK_DATA_SIZE;
			} else {
				s.last_block = i;
				s.data.insert(s.data.end(), block.data, block.data + sizeof(block.data));
			}
		}
	}

	versions_t valid_versions;

	for (versions_t::const_iterator it = all_versions.begin(),
			end = all_versions.end();
		it != end; it++) {
		const std::vector<uint8_t>& data = it->second.data;

		uint32_t len = (data[0] << 24) | (data[1] << 16) |
			(data[2] << 8) | (data[3] << 0);
		if (len > data.size())
			continue;

		uint32_t crc = (data[len - 4] << 24) | (data[len - 3] << 16) |
			(data[len - 2] << 8) | (data[len - 1] << 0);

		if (crc != crc_stream(&data[0], len - 4))
			continue;

		stream& valid = valid_versions[it->second.version];

		valid = it->second;
		valid.data.clear();
		valid.data.insert(
			valid.data.begin(),
			data.begin() + 4,
			data.begin() + len - 4);
	}

	return valid_versions;
}

boost::optional<Eeprom::stream> Eeprom::read_latest_version()
{
	versions_t versions = read_valid_versions();

	if (!versions.size())
		return boost::optional<stream>();

	std::vector<uint16_t> version_numbers;
	for (versions_t::const_iterator it = versions.begin(), end = versions.end(); it != end; it++)
		version_numbers.push_back(it->first);

	std::sort(version_numbers.begin(), version_numbers.end());
	for (std::vector<uint16_t>::const_iterator it = version_numbers.begin(),
			end = version_numbers.end();
		it != end; it++) {
		if (it + 1 != end && *it + 2 <= *(it + 1)) {
			return versions[*it];
		}
	}

	return boost::make_optional(versions[version_numbers.back()]);
}



boost::optional<std::vector<uint8_t> > Eeprom::read_stream()
{
	boost::optional<stream> stream = read_latest_version();

	if (!stream)
		return boost::optional<std::vector<uint8_t> >();

	return boost::make_optional(stream->data);
}




#include <iostream>
#include <boost/format.hpp>

void dump_block(const void* block)
{
	const uint8_t* p = reinterpret_cast<const uint8_t*>(block);

	for (int j = 0; j < 64; j += 16) {
		std::cout << boost::format("%|08x| ") % j;

		for (int k = 0; k < 16; k++)
			std::cout << boost::format(" %|02x|") % int(p[j + k]);

		std::cout << "  ";

		for (int k = 0; k < 16; k++)
			if (isprint(p[j + k]))
				std::cout << p[j + k];
			else
				std::cout << ".";

		std::cout << std::endl;
	}
}

void Eeprom::write_stream(const std::vector<uint8_t>& data)
{
	boost::optional<Eeprom::stream> last = read_latest_version();

	uint16_t version = last ? last->version + 1 : 0;
	uint16_t first_block = last ? last->last_block + 1 : 0;

	std::vector<uint8_t> stream;
	uint32_t stream_size = data.size() + 8;

	stream.reserve(stream_size);
	stream.push_back((stream_size >> 24) & 0xff);
	stream.push_back((stream_size >> 16) & 0xff);
	stream.push_back((stream_size >>  8) & 0xff);
	stream.push_back((stream_size >>  0) & 0xff);
	stream.insert(stream.end(), data.begin(), data.end());

	uint32_t crc = crc_stream(&stream[0], stream.size());
	stream.push_back((crc >> 24) & 0xff);
	stream.push_back((crc >> 16) & 0xff);
	stream.push_back((crc >>  8) & 0xff);
	stream.push_back((crc >>  0) & 0xff);

	std::vector<eeprom_block> blocks;

	for (size_t offset = 0; offset < stream.size(); offset += BLOCK_DATA_SIZE) {
		eeprom_block block;

		size_t len = std::min<size_t>(stream.size() - offset, BLOCK_DATA_SIZE);

		block.version = htobe16(version);
		memcpy(block.data, &stream[offset], len);
		memset(block.data + len, 0, BLOCK_DATA_SIZE - len);
		block.crc = htobe16(crc_block(&block, sizeof(block) - sizeof(uint16_t)));

		blocks.push_back(block);
	}

	if (blocks.size() > BLOCK_COUNT / 2)
		throw std::runtime_error("stream too large");

	for (size_t idx = 0; idx < blocks.size(); idx++) {
		uint16_t block_idx = (first_block + idx) % BLOCK_COUNT;

		write_block(block_idx, blocks[idx]);
	}

	_blocks.clear();
}

void Eeprom::dump_contents()
{
	std::vector<eeprom_block>& blocks = read_all_blocks();

	for (size_t i = 0; i < blocks.size(); i++) {
		std::cout << "Block " << i << " ("
			<< (crc_block(&blocks[i], BLOCK_SIZE) ? "in" : "")
			<< "valid)"
			<< std::endl;

		dump_block(&blocks[i]);
	}
}
