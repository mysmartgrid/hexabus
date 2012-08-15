#include "main_window.hpp"

using namespace hexadisplay;

MainWindow::MainWindow(
    ValueProvider::Ptr value_provider, 
    SwitchDevice::Ptr switch_device
  ) : _value_provider(value_provider)
  , _switch_device(switch_device)
  , _ui(new Ui::SimpleUI)
{
  _ui->setupUi(this);
  _ui->off_PB->setEnabled(false);
}

MainWindow::~MainWindow() {
  delete _ui;
}

void MainWindow::on_refresh_PB_clicked() {
  try {
    _ui->temperature_display->setText(_value_provider->get_temperature());
    _ui->pressure_display->setText(_value_provider->get_pressure());
    _ui->humidity_display->setText(_value_provider->get_humidity());
//    _ui->power_display->setText(_value_provider->get_power());
  } catch (klio::StoreException const& ex) {
    std::cout << "Failed to access store: " << ex.what() << std::endl;
    exit(-1);
  }
}

void MainWindow::on_on_PB_clicked() {
  std::cout << "Switching on..." << std::endl;
  _switch_device->on();
  _ui->on_PB->setEnabled(false);
  _ui->off_PB->setEnabled(true);
}

void MainWindow::on_off_PB_clicked() {
  std::cout << "Switching off..." << std::endl;
  _switch_device->off();
  _ui->on_PB->setEnabled(true);
  _ui->off_PB->setEnabled(false);
}

void MainWindow::on_toggle_PB_clicked() {
  std::cout << "Toggling ..." << std::endl;
  _switch_device->toggle();
  if (_switch_device->is_on()) {
    _ui->on_PB->setEnabled(false);
    _ui->off_PB->setEnabled(true);

  } else {
    _ui->on_PB->setEnabled(true);
    _ui->off_PB->setEnabled(false);
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
  std::cout << "Goodbye." << std::endl;
}
