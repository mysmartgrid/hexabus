#include "liveness.hpp"

#include <boost/thread/thread.hpp>

#include "../../shared/endpoints.h"
#include "socket.hpp"
#include <boost/bind.hpp>

using namespace hexabus;

LivenessReporter::LivenessReporter(Socket& socket, boost::posix_time::time_duration interval)
	: _socket(socket),
		_interval(interval),
		_timer(socket.ioService())
{
}

void LivenessReporter::interval(boost::posix_time::time_duration interval)
{
	_interval = interval;

	if (_running) {
		stop();
		start();
	}
}

void LivenessReporter::start()
{
	armTimer();
	_running = true;
}

void LivenessReporter::stop()
{
	_timer.cancel();
	_running = false;
}

void LivenessReporter::reportAlive(bool alive)
{
	_socket.send(InfoPacket<bool>(EP_LIVENESS, alive));
}

void LivenessReporter::establishPaths(unsigned int hopCount)
{
	for (; hopCount > 0; hopCount--) {
		reportAlive();

		boost::this_thread::sleep(boost::posix_time::seconds(1));
	}
}

void LivenessReporter::armTimer()
{
	_timer.expires_from_now(_interval);
	_timer.async_wait(boost::bind(&LivenessReporter::waitHandler, this, boost::asio::placeholders::error));
}

void LivenessReporter::waitHandler(const boost::system::error_code& e)
{
	if (e == boost::asio::error::operation_aborted) {
		return;
	}
	reportAlive(true);
	armTimer();
}
