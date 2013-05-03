/**
 * This file is part of libhexanode.
 *
 * (c) Fraunhofer ITWM - Mathias Dalheimer <dalheimer@itwm.fhg.de>, 2010
 *
 * libhexanode is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libhexanode is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libhexanode. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LIBKLIO_ERROR_HPP
#define LIBKLIO_ERROR_HPP 1

#include <string>
#include <exception>

namespace hexanode {
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

  class CommunicationException : public GenericException{
    public:
      typedef std::tr1::shared_ptr<CommunicationException> Ptr;
      CommunicationException (const std::string reason) :
        hexanode::GenericException(reason) {};
      virtual ~CommunicationException() throw() {};

  };

  class DataFormatException : public GenericException{
    public:
      typedef std::tr1::shared_ptr<DataFormatException> Ptr;
      DataFormatException (const std::string reason) :
        hexanode::GenericException(reason) {};
      virtual ~DataFormatException() throw() {};

  };

  class MidiException : public GenericException{
    public:
      typedef std::tr1::shared_ptr<MidiException> Ptr;
      MidiException (const std::string reason) :
        hexanode::GenericException(reason) {};
      virtual ~MidiException() throw() {};
  };

  class NotFoundException : public GenericException{
    public:
      typedef std::tr1::shared_ptr<NotFoundException> Ptr;
      NotFoundException (const std::string reason) :
        hexanode::GenericException(reason) {};
      virtual ~NotFoundException() throw() {};
  };

  class DuplicateException : public GenericException{
    public:
      typedef std::tr1::shared_ptr<DuplicateException> Ptr;
      DuplicateException (const std::string reason) :
        hexanode::GenericException(reason) {};
      virtual ~DuplicateException() throw() {};
  };

}

#endif /* LIBKLIO_ERROR_HPP */
