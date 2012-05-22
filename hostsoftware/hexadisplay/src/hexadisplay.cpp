/**
 * This file is part of hexadisplay.
 *
 * (c) Fraunhofer ITWM - Mathias Dalheimer <dalheimer@itwm.fhg.de>, 2012
 *
 * hexadisplay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * hexadisplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with hexadisplay. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <QApplication>
#include <libhexadisplay/common.hpp>
#include <libhexadisplay/main_window.hpp>
#include <iostream>

#include <boost/uuid/uuid_io.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/positional_options.hpp>
namespace po = boost::program_options;



int main (int argc, char* argv[]) {
  try {
    std::ostringstream oss;
    oss << "Usage: " << argv[0] << " ACTION [additional options]";
    po::options_description desc(oss.str());
    desc.add_options()
      ("help,h", "produce help message")
      ("version,v", "print version and exit")
      ("configfile,s", po::value<std::string>(), "the data store to use")
      ;
    po::positional_options_description p;
    p.add("configfile", 1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
        options(desc).positional(p).run(), vm);
    po::notify(vm);

    // Begin processing of commandline parameters.
    std::string configfile;

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 1;
    }

    if (vm.count("version")) {
      hexadisplay::VersionInfo::Ptr version(new hexadisplay::VersionInfo());
      std::cout << "HexaDisplay version " << version->getVersion() << std::endl;
      return 0;
    }

    if (! vm.count("configfile")) {
      std::cerr << "You must specify a configuration file." << std::endl;
      return 1;
    } else {
      configfile=vm["configfile"].as<std::string>();
    }

    try {
      QApplication app(argc, argv);
      hexadisplay::ValueProvider::Ptr valueProvider(
          new hexadisplay::ValueProvider(configfile));
      hexadisplay::SwitchDevice::Ptr switchDevice(
          new hexadisplay::SwitchDevice(configfile));
      hexadisplay::MainWindow mainWindow(
          valueProvider, switchDevice);
      mainWindow.show();
      return app.exec();

    } catch (klio::StoreException const& ex) {
      std::cout << "Failed to access store: " << ex.what() << std::endl;
      exit(-1);
    }

  } catch(std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  } catch(...) {
    std::cerr << "Exception of unknown type!" << std::endl;
    return 1;
  }
}
