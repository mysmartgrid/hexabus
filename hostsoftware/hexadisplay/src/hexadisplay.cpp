#include <QApplication>
#include <QLabel>
#include <common.hpp>
#include <iostream>

int main (int argc, char* argv[]) {
  hexadisplay::VersionInfo::Ptr version(new hexadisplay::VersionInfo());
  std::cout << "HexaDisplay V" << version->getVersion() << std::endl;

  QApplication app(argc, argv);
  QLabel testLabel("foobar");
  testLabel.show();

  return app.exec();
}

