#ifndef TYPES_HPP_
#define TYPES_HPP_

#include <iostream>
#include <string>
#include <stdint.h>
#include <sstream>
#include <iomanip>

class Device {
	private:
		std::string _iface;
		uint16_t _pan_id;
		uint16_t _short_addr;
		uint64_t _hwaddr;
		uint32_t _frame_ctr;

	public:
		Device(const std::string& iface, uint64_t hwaddr, uint32_t frame_ctr)
			: _iface(iface), _pan_id(0), _short_addr(0xfffe), _hwaddr(hwaddr), _frame_ctr(frame_ctr)
		{}

		Device(const std::string& iface, uint16_t pan_id, uint16_t short_addr, uint64_t hwaddr, uint32_t frame_ctr)
			: _iface(iface), _pan_id(pan_id), _short_addr(short_addr), _hwaddr(hwaddr), _frame_ctr(frame_ctr)
		{}

		const std::string& iface() const { return _iface; }
		uint16_t pan_id() const { return _pan_id; }
		uint16_t short_addr() const { return _short_addr; }
		uint64_t hwaddr() const { return _hwaddr; }
		uint32_t frame_ctr() const { return _frame_ctr; }
};

template<class CharT, class Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Device& dev)
{
	std::stringstream ss;

	ss << "Device on " << dev.iface() << std::endl
		<< "	PAN ID " << std::hex << std::setw(4) << std::setfill('0') << dev.pan_id() << std::endl
		<< "	Short Address " << dev.short_addr() << std::endl
		<< "	Extendded address ";
       
	uint8_t ext[8];
	uint64_t hw = dev.hwaddr();
	memcpy(ext, &hw, 8);
	for (int i = 0; i < 8; i++) {
		if (i)
			ss << ":";
		ss << std::setw(2) << int(ext[i]);
	}
	ss << std::endl;
	ss << "	Frame Counter " << std::dec << dev.frame_ctr() << std::endl;

	return os << ss.str();
}

#endif
