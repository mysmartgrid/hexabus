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

  class CaseNotImplementedException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<CaseNotImplementedException> Ptr;
      CaseNotImplementedException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~CaseNotImplementedException() throw() {};
  };

  class VertexNotFoundException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<VertexNotFoundException> Ptr;
      VertexNotFoundException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~VertexNotFoundException() throw() {};
  };

  class EdgeLinkException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<EdgeLinkException> Ptr;
      EdgeLinkException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~EdgeLinkException() throw() {};
  };

  class StateNameNotFoundException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<StateNameNotFoundException> Ptr;
      StateNameNotFoundException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~StateNameNotFoundException() throw() {};
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

  class DuplicateEntryException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<DuplicateEntryException> Ptr;
      DuplicateEntryException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~DuplicateEntryException() throw() {};
  };

  class MissingEntryException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<MissingEntryException> Ptr;
      MissingEntryException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~MissingEntryException() throw() {};
  };
}


#endif // LIBHBC_ERROR_HPP
