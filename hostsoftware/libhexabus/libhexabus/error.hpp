#ifndef LIBHEXABUS_ERROR_HPP
#define LIBHEXABUS_ERROR_HPP 1

#include <string>
#include <exception>
#include <memory>
#include <boost/system/error_code.hpp>

namespace hexabus {

class GenericException : public std::exception {
public:
	GenericException(std::string reason)
		: _reason(std::move(reason))
	{
	}

	virtual ~GenericException() {};

	virtual const char* what() const noexcept { return reason().c_str(); }

	virtual const std::string& reason() const { return _reason; }

private:
	std::string _reason;
};

class NetworkException : public GenericException {
public:
	NetworkException (const std::string reason, const boost::system::error_code& code) :
		hexabus::GenericException(reason), _code(code) {};

	boost::system::error_code code() const { return _code; }

private:
	boost::system::error_code _code;
};

class BadPacketException : public GenericException {
public:
	using GenericException::GenericException;
};

}

#endif /* LIBHEXABUS_ERROR_HPP */
