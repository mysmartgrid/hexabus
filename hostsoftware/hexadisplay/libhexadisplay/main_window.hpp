#ifndef LIBHEXADISPLAY_MAIN_WINDOW_HPP
#define LIBHEXADISPLAY_MAIN_WINDOW_HPP 1

#include <QtGui>
#include <QVBoxLayout>
#include <libhexadisplay/common.hpp>
#include <libhexadisplay/value_provider.hpp>
#include <libhexadisplay/switch_device.hpp>
#include <libhexadisplay/ui_simple.h>

namespace hexadisplay {

  class MainWindow : public QWidget, private Ui::SimpleUI {
    Q_OBJECT 

    public:
      typedef std::tr1::shared_ptr<MainWindow> Ptr;
      MainWindow (
          hexadisplay::ValueProvider::Ptr value_provider,
          hexadisplay::SwitchDevice::Ptr switch_device
          );
      virtual ~MainWindow();
    private slots:
      void on_refresh_PB_clicked();
      void on_on_PB_clicked();
      void on_off_PB_clicked();

    protected:
      void closeEvent(QCloseEvent *event);
    private:
      MainWindow (const MainWindow& original);
      MainWindow& operator= (const MainWindow& rhs);
      hexadisplay::ValueProvider::Ptr _value_provider;
      hexadisplay::SwitchDevice::Ptr _switch_device;
      Ui::SimpleUI* _ui;
      //QVBoxLayout* _vbox;
      //QLabel* _temperature;
      //QLabel* _pressure;
      //QLabel* _humidity;
      //QLabel* _power;
      //QPushButton* _refresh;

  };

};


#endif /* LIBHEXADISPLAY_MAIN_WINDOW_HPP */

