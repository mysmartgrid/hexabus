/**
 * This file is part of libhexabus.
 *
 * (c) Fraunhofer ITWM - Mathias Dalheimer <dalheimer@itwm.fhg.de>, 2011
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
#include "common.hpp"
#include "config.h"
#include <sstream>

using namespace hexanode;

std::string VersionInfo::get_version() {
  std::ostringstream oss;
  oss << LIBHEXANODE_VERSION_MAJOR << "." << LIBHEXANODE_VERSION_MINOR << "." << LIBHEXANODE_VERSION_PATCH;
  return oss.str();
}

