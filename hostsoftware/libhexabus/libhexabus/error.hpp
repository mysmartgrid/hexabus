#ifndef LIBHEXABUS_ERROR_HPP
#define LIBHEXABUS_ERROR_HPP 1

#include <string>
#include <exception>
#include <boost/system/error_code.hpp>

namespace hexabus {
  class GenericException : public std::exception {
    public:
      typedef std::tr1::shared_ptr<GenericException> Ptr;
      GenericException (const std::string reason) : _reason(reason) {};
      virtual ~GenericException() throw() {};
      virtual const char* what() const throw() { return reason().c_str(); }
      virtual const std::string& reason() const { return _reason; }

    private:
      std::string _reason;
  };

  class NetworkException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<NetworkException> Ptr;
      NetworkException (const std::string reason, const boost::system::error_code& code) :
        hexabus::GenericException(reason), _code(code) {};
      virtual ~NetworkException() throw() {};

      boost::system::error_code code() const { return _code; }

    private:
      boost::system::error_code _code;
  };

}

#endif /* LIBHEXABUS_ERROR_HPP */
