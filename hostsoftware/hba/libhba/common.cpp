/**
 * This file is part of hexabus assembler.
 *
 * (c) Fraunhofer ITWM - Mathias Dalheimer <dalheimer@itwm.fhg.de>, 2011
 *
 * hexabus assembler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * hexabus assembler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hexabus assembler. If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "common.hpp"
#include <sstream>

using namespace hexabus;

const std::string VersionInfo::getVersion() {
  std::ostringstream oss;
  oss << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_PATCH;
  return oss.str();
}
