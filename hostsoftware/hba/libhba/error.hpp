/**
 * This file is part of libhexabus.
 *
 * (c) Fraunhofer ITWM - Mathias Dalheimer <dalheimer@itwm.fhg.de>, 2010
 *
 * libhexabus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libhexabus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libhexabus. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LIBHBA_ERROR_HPP
#define LIBHBA_ERROR_HPP 1

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

  class DatatypeNotFoundException : public GenericException{
	public:
	  typedef std::tr1::shared_ptr<DatatypeNotFoundException> Ptr;
	  DatatypeNotFoundException (const std::string reason) :
		hexabus::GenericException(reason) {};
	  virtual ~DatatypeNotFoundException() throw() {};

  };

  class EdgeLinkException : public GenericException{
	public:
	  typedef std::tr1::shared_ptr<EdgeLinkException> Ptr;
	  EdgeLinkException (const std::string reason) :
		hexabus::GenericException(reason) {};
	  virtual ~EdgeLinkException() throw() {};

  };

  class VertexNotFoundException : public GenericException{
	public:
	  typedef std::tr1::shared_ptr<VertexNotFoundException> Ptr;
	  VertexNotFoundException (const std::string reason) :
		hexabus::GenericException(reason) {};
	  virtual ~VertexNotFoundException() throw() {};

  };

  class UnreachableException : public GenericException{
	public:
	  typedef std::tr1::shared_ptr<UnreachableException> Ptr;
	  UnreachableException (const std::string reason) :
		hexabus::GenericException(reason) {};
	  virtual ~UnreachableException() throw() {};

  };


  class UnleavableException : public GenericException{
	public:
	  typedef std::tr1::shared_ptr<UnleavableException> Ptr;
	  UnleavableException (const std::string reason) :
		hexabus::GenericException(reason) {};
	  virtual ~UnleavableException() throw() {};

  };


}

#endif /* LIBHBA_ERROR_HPP */
