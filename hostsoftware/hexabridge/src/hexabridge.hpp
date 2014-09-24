/**
 * This file is part of hexabridge.
 *
 * (c) Fraunhofer ITWM - Stephan Platz <platz@itwm.fhg.de>, 2014
 *
 * hexabridge is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * hexabridge is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hexabridge. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _HEXABRIDGE_HPP
#define _HEXABRIDGE_HPP

#include <boost/asio/io_service.hpp>

#include <libhexabus/device.hpp>

namespace hexabridge {
	class Pusher {
		public:
			typedef boost::shared_ptr<Pusher> Ptr;
			Pusher(boost::asio::io_service& io, const std::vector<std::string>& interfaces, const std::vector<std::string>& addresses, const std::string& url, const std::string& id, const std::string& token, int interval = 60, bool debug = false);
			virtual ~Pusher() {};

			std::string loadDeviceName();
			void saveDeviceName(const std::string& name);

			uint32_t getLastReading();

		private:
			void _init();

		private:
			hexabus::Device _device;
			const std::string _url, _id, _token;
			bool _debug;
	};
}

#endif // _HEXABRIDGE_HPP
