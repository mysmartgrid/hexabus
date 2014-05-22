//
// daemon.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio/io_service.hpp>
//#include <boost/asio/ip/udp.hpp>
#include <boost/asio/signal_set.hpp>
//#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/program_options.hpp>
//#include <ctime>
#include <iostream>
#include <syslog.h>
//#include <unistd.h>

namespace po = boost::program_options;

#include "hexabus_server.hpp"

boost::asio::io_service io_service;

int main(int argc, char** argv)
{
  std::ostringstream oss;
  oss << "Usage: " << argv[0] << " [additional options]";
  po::options_description desc(oss.str());
  desc.add_options()
    ("help,h", "produce this message")
    ("debug,d", "enable debug mode")
    ("logfile,l", po::value<std::string>(), "set the logfile to use")
    ("interval,i", po::value<int>(), "set the broadcast interval")
    ("interface,I", po::value<std::string>(), "interface to use for multicast")
    ("address,a", po::value<std::string>(), "address to listen on")
    ;
  po::variables_map vm;

  // Begin processing of commandline parameters.
  try {
    po::store(po::command_line_parser(argc, argv).
        options(desc).run(), vm);
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "Cannot process commandline options: " << e.what() << std::endl;
    exit(-1);
  }

  bool debug = false;
  std::string logfile = "/tmp/hexadaemon.log";
  int interval = 2;
  std::string interface;
  std::string address;

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  }

  if (vm.count("debug")) {
    debug = true;
  }

  if (vm.count("logfile")) {
    logfile = vm["logfile"].as<std::string>();
  }

  if (vm.count("interval")) {
    interval = vm["interval"].as<int>();
  }

  if (vm.count("interface")) {
    interface = vm["interface"].as<std::string>();
  } else {
    std::cerr << "You have to specify at least one interface." << std::endl;
    return 1;
  }


  if (vm.count("address")) {
    address = vm["address"].as<std::string>();
  } else {
    std::cerr << "You have to specify at least one address." << std::endl;
    return 1;
  }

  try
  {

    // Initialise the server before becoming a daemon. If the process is
    // started from a shell, this means any errors will be reported back to the
    // user.
    //udp_daytime_server server(io_service);
    hexadaemon::HexabusServer *server;
    server = new hexadaemon::HexabusServer(io_service, interface, address, interval, debug);

    // Register signal handlers so that the daemon may be shut down. You may
    // also want to register for other signals, such as SIGHUP to trigger a
    // re-read of a configuration file.
    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait(
        boost::bind(&boost::asio::io_service::stop, &io_service));

    if ( !debug ) {
    // Inform the io_service that we are about to become a daemon. The
    // io_service cleans up any internal resources, such as threads, that may
    // interfere with forking.
    io_service.notify_fork(boost::asio::io_service::fork_prepare);

    // Fork the process and have the parent exit. If the process was started
    // from a shell, this returns control to the user. Forking a new process is
    // also a prerequisite for the subsequent call to setsid().
    if (pid_t pid = fork())
    {
      if (pid > 0)
      {
        // We're in the parent process and need to exit.
        //
        // When the exit() function is used, the program terminates without
        // invoking local variables' destructors. Only global variables are
        // destroyed. As the io_service object is a local variable, this means
        // we do not have to call:
        //
        //   io_service.notify_fork(boost::asio::io_service::fork_parent);
        //
        // However, this line should be added before each call to exit() if
        // using a global io_service object. An additional call:
        //
        //   io_service.notify_fork(boost::asio::io_service::fork_prepare);
        //
        // should also precede the second fork().
        exit(0);
      }
      else
      {
        syslog(LOG_ERR | LOG_USER, "First fork failed: %m");
        return 1;
      }
    }

    // Make the process a new session leader. This detaches it from the
    // terminal.
    setsid();

    // A process inherits its working directory from its parent. This could be
    // on a mounted filesystem, which means that the running daemon would
    // prevent this filesystem from being unmounted. Changing to the root
    // directory avoids this problem.
    chdir("/");

    // The file mode creation mask is also inherited from the parent process.
    // We don't want to restrict the permissions on files created by the
    // daemon, so the mask is cleared.
    umask(0);

    // A second fork ensures the process cannot acquire a controlling terminal.
    if (pid_t pid = fork())
    {
      if (pid > 0)
      {
        exit(0);
      }
      else
      {
        syslog(LOG_ERR | LOG_USER, "Second fork failed: %m");
        return 1;
      }
    }

    // Close the standard streams. This decouples the daemon from the terminal
    // that started it.
    close(0);
    close(1);
    close(2);

    // We don't want the daemon to have any standard input.
    if (open("/dev/null", O_RDONLY) < 0)
    {
      syslog(LOG_ERR | LOG_USER, "Unable to open /dev/null: %m");
      return 1;
    }

    // Send standard output to a log file.
    const char* output = logfile.c_str();
    const int flags = O_WRONLY | O_CREAT | O_APPEND;
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    if (open(output, flags, mode) < 0)
    {
      syslog(LOG_ERR | LOG_USER, "Unable to open output file %s: %m", output);
      return 1;
    }

    // Also send standard error to the same log file.
    if (dup(1) < 0)
    {
      syslog(LOG_ERR | LOG_USER, "Unable to dup output descriptor: %m");
      return 1;
    }

    // Inform the io_service that we have finished becoming a daemon. The
    // io_service uses this opportunity to create any internal file descriptors
    // that need to be private to the new process.
    io_service.notify_fork(boost::asio::io_service::fork_child);

    // The io_service can now be used normally.
    syslog(LOG_INFO | LOG_USER, "HexabusDaemon started");
    }
    io_service.run();
    if ( !debug )
    syslog(LOG_INFO | LOG_USER, "HexabusDaemon stopped");
  }
  catch (std::exception& e)
  {
    syslog(LOG_ERR | LOG_USER, "Exception: %s", e.what());
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}
