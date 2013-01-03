#ifndef LIBHEXABUS_LIVENESS_HPP
#define LIBHEXABUS_LIVENESS_HPP 1

#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "socket.hpp"

namespace hexabus {
	class LivenessReporter {
		private:
			Socket& _socket;
			boost::posix_time::time_duration _interval;
			boost::asio::deadline_timer _timer;
			bool _running;

			void armTimer();
			void waitHandler(const boost::system::error_code& e);

		public:
			LivenessReporter(Socket& socket, boost::posix_time::time_duration interval = boost::posix_time::minutes(60));

			boost::posix_time::time_duration interval() const { return _interval; }
			void interval(boost::posix_time::time_duration interval);

			void start();
			void stop();

			void reportAlive(bool alive = true);

			void establishPaths(unsigned int hopCount);
	};
}

#endif
