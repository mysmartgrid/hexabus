#ifndef LIBHBC_ERROR_HPP
#define LIBHBC_ERROR_HPP

#include <string>
#include <exception>

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

  class NoInitStateException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<NoInitStateException> Ptr;
      NoInitStateException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~NoInitStateException() throw() {};
  };

  class NonexistentStateException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<NonexistentStateException> Ptr;
      NonexistentStateException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~NonexistentStateException() throw() {};
  };
}


#endif // LIBHBC_ERROR_HPP
