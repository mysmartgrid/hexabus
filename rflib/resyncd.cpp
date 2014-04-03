#include "resyncd.hpp"

#include "bootstrap.hpp"

#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/unistd.h>
#include <poll.h>

#include <stdexcept>
#include <ostream>

#include <libdaemon/daemon.h>

namespace {

void die(const std::string& str, bool err = false, int code = 0)
{
	if (err)
		throw std::runtime_error(str + ": " + strerror(errno));
	else
		throw std::runtime_error(str);
}

void exit_d()
{
	daemon_signal_done();
	daemon_pid_file_remove();
	exit(0);
}

void die_d(const std::string& str, bool err = false, int code = -1)
{
	if (err)
		daemon_log(LOG_ERR, (str + ": " + strerror(errno)).c_str());
	else
		daemon_log(LOG_ERR, str.c_str());

	if (code >= 0)
		daemon_retval_send(code);
	exit_d();
}

void log_d(int level, const std::string& str)
{
	daemon_log(level, str.c_str());
}

void log_std(int level, const std::string& str)
{
	if (level == LOG_INFO)
		std::cout << str << std::endl;
	else
		std::cerr << str << std::endl;
}

}



struct poll_err {};

void resyncd_child(const std::string& iface, bool daemonize)
{
	void (*die)(const std::string& str, bool iserr, int code);
	void (*log)(int level, const std::string& str);

	if (daemonize) {
		die = ::die_d;
		log = ::log_d;
	} else {
		die = ::die;
		log = ::log_std;
	}

	if (daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, 0) < 0)
		die("failed to register sighandlers", true, 3);

	if (daemonize)
		daemon_retval_send(0);

	int sigfd = daemon_signal_fd();

	pollfd fds[2];

	fds[0].fd = sigfd;
	fds[0].events = POLLIN;

	for (;;) {
		try {
			ResyncHandler rh(iface);

			fds[1].fd = rh.fd();
			fds[1].events = POLLIN;

			while (true) {
				int rc = poll(fds, 2, -1);

				if (rc < 0 && errno == EINTR) {
					continue;
				} else if (rc < 0) {
					throw poll_err();
				}

				if (fds[0].revents & POLLIN) {
					int sig = daemon_signal_next();

					if (sig < 0)
						die("signal processing failed", true, -1);

					switch (sig) {
					case SIGINT:
					case SIGTERM:
					case SIGQUIT:
						log(LOG_INFO, "shutting down");
						return;

					default:
						die("unknown signal received", false, -1);
						return;
					}
				}
				if (fds[1].revents & POLLIN) {
					rh.run_once();
				}
			}
		} catch (const poll_err& e) {
			log(LOG_ERR, std::string("poll failed: ") + strerror(errno));
		} catch (const std::exception& e) {
			log(LOG_ERR, std::string("error: ") + e.what());
		} catch (...) {
			log(LOG_ERR, "failed: unknown error");
		}
		poll(0, 0, 10000);
	}
}

void run_resyncd(const std::string& iface)
{
	std::string ident = "hxbnm-resyncd-" + iface;

	daemon_pid_file_ident = ident.c_str();
	daemon_log_ident = ident.c_str();
    
	if (daemon_reset_sigs(-1) < 0)
		die("failed to reset signal handlers");

	if (daemon_unblock_sigs(-1) < 0)
		die("failed to unblock signals");

	pid_t daemon_pid = daemon_pid_file_is_running();
	if (daemon_pid >= 0)
		die("already running");

	if (daemon_retval_init() < 0)
		die("init failed");

	daemon_pid = daemon_fork();
	if (daemon_pid < 0) {
		daemon_retval_done();
		die("init failed");
	}

	if (daemon_pid) {
		int ret = daemon_retval_wait(10);

		if (ret < 0)
			die("child communication failed");
		if (ret > 0)
			die("start failed, check logs");
	} else {
		if (daemon_close_all(-1) < 0)
			die_d("failed to close fds", true, 1);

		if (daemon_pid_file_create() < 0)
			die_d("failed to create PID file", true,  2);

		resyncd_child(iface, true);

		exit_d();
	}
}

void run_resyncd_sync(const std::string& iface)
{
	resyncd_child(iface, false);
}

void kill_resyncd(const std::string& iface)
{
	std::string ident = "hxbnm-resyncd-" + iface;

	daemon_pid_file_ident = ident.c_str();
	daemon_log_ident = ident.c_str();

	int rc = daemon_pid_file_kill_wait(SIGTERM, 5);
	if (rc < 0)
		die("kill failed", true);
}
