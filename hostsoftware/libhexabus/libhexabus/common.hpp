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

#ifndef LIBHEXABUS_COMMON_HPP
#define LIBHEXABUS_COMMON_HPP 1

/* Include TR1 shared ptrs in a portable way. */
#include <cstddef> // for __GLIBCXX__
#ifdef __GLIBCXX__
#  include <tr1/memory>
#else
#  ifdef __IBMCPP__
#    define __IBMCPP_TR1__
#  endif
#  include <memory>
#endif

//#define ENABLE_LOGGING 0
#include <config.h>

#ifdef ENABLE_LOGGING
#include <iostream>
#ifdef LOG
# undef LOG
#endif
#define LOG(msg) std::cout << msg << std::endl;
#else
#define LOG(msg)
#endif

#include <stdint.h>
#include <string>
#include <libhexabus/compat.hpp>

namespace hexabus {
	const char* version();
};

#endif /* LIBHEXABUS_COMMON_HPP */
