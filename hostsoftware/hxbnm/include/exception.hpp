#ifndef EXCEPTION_HPP_
#define EXCEPTION_HPP_

#include <exception>
#include <stdexcept>

#include <string.h>
#include <stdio.h>
#include <errno.h>

namespace hxbnm {

class system_error : public std::runtime_error {
	public:
		system_error(const std::string& from, const std::string& what)
			: runtime_error(from + " " + what  + ": " + strerror(errno))
		{
		}
};

class stream_error : public std::runtime_error {
	public:
		stream_error(const std::string& from, const std::string& what)
			: runtime_error(from + ": " + what)
		{
		}
};

}

#define HXBNM_THROW(type, ...) do { throw ::hxbnm::type ## _error(__PRETTY_FUNCTION__, __VA_ARGS__); } while (0)

#endif
