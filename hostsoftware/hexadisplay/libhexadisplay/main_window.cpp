#include "main_window.hpp"

using namespace hexadisplay;

MainWindow::MainWindow(ValueProvider::Ptr value_provider) 
  : _value_provider(value_provider)
  , _vbox(new QVBoxLayout(this))
  , _temperature(new QLabel("temperature"))
  , _pressure(new QLabel("pressure"))
  , _humidity(new QLabel("humidity"))
  , _power(new QLabel("power"))
  , _refresh(new QPushButton("refresh"))
{
  
  _vbox->addWidget(_temperature);
  _vbox->addWidget(_pressure);
  _vbox->addWidget(_humidity);
  _vbox->addWidget(_power);
  _vbox->addWidget(_refresh);
  this->setLayout(_vbox);

  connect(_refresh, SIGNAL(pressed()),
             this, SLOT(updateValues()));
}

MainWindow::~MainWindow() {
  delete _refresh;
  delete _power;
  delete _humidity;
  delete _pressure;
  delete _temperature;
  delete _vbox;
}

void MainWindow::updateValues() {
  try {
    _temperature->setText(_value_provider->get_temperature());
    _pressure->setText(_value_provider->get_pressure());
    _humidity->setText(_value_provider->get_humidity());
    _power->setText(_value_provider->get_power());
  } catch (klio::StoreException const& ex) {
    std::cout << "Failed to access store: " << ex.what() << std::endl;
    exit(-1);
  }

}

void MainWindow::closeEvent(QCloseEvent *event) {
  std::cout << "Goodbye." << std::endl;
}
