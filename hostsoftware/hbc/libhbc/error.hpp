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

  class GraphSimplificationException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<GraphSimplificationException> Ptr;
      GraphSimplificationException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~GraphSimplificationException() throw() {};
  };

  class GraphOutputException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<GraphOutputException> Ptr;
      GraphOutputException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~GraphOutputException() throw() {};
  };

  class FileNotFoundException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<FileNotFoundException> Ptr;
      FileNotFoundException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~FileNotFoundException() throw() {};
  };

  class PlaceholderInInstanceException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<PlaceholderInInstanceException> Ptr;
      PlaceholderInInstanceException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~PlaceholderInInstanceException() throw() {};
  };

  class GraphTransformationErrorException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<GraphTransformationErrorException> Ptr;
      GraphTransformationErrorException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~GraphTransformationErrorException() throw() {};
  };

  class EndpointNotBroadcastException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<EndpointNotBroadcastException> Ptr;
      EndpointNotBroadcastException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~EndpointNotBroadcastException() throw() {};
  };

  class EndpointNotWriteableException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<EndpointNotWriteableException> Ptr;
      EndpointNotWriteableException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~EndpointNotWriteableException() throw() {};
  };

  class HBAConversionErrorException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<HBAConversionErrorException> Ptr;
      HBAConversionErrorException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~HBAConversionErrorException() throw() {};
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

  class InvalidPlaceholderException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<InvalidPlaceholderException> Ptr;
      InvalidPlaceholderException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~InvalidPlaceholderException() throw() {};
  };

  class InvalidParameterTypeException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<InvalidParameterTypeException> Ptr;
      InvalidParameterTypeException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~InvalidParameterTypeException() throw() {};
  };

  class StateNameNotFoundException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<StateNameNotFoundException> Ptr;
      StateNameNotFoundException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~StateNameNotFoundException() throw() {};
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

  class ModuleNotFoundException : public GenericException {
    public:
      typedef std::tr1::shared_ptr<ModuleNotFoundException> Ptr;
      ModuleNotFoundException (const std::string reason) : hexabus::GenericException(reason) {};
      virtual ~ModuleNotFoundException() throw() {};
  };
}


#endif // LIBHBC_ERROR_HPP
