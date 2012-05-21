#ifndef LIBHEXADISPLAY_MAIN_WINDOW_HPP
#define LIBHEXADISPLAY_MAIN_WINDOW_HPP 1

#include <QtGui>
#include <QVBoxLayout>
#include <libhexadisplay/common.hpp>
#include <libhexadisplay/value_provider.hpp>

namespace hexadisplay {

  class MainWindow : public QWidget{
    Q_OBJECT 

    public:
      typedef std::tr1::shared_ptr<MainWindow> Ptr;
      MainWindow (hexadisplay::ValueProvider::Ptr value_provider);
      virtual ~MainWindow();
    public slots:
      void updateValues();
    protected:
      void closeEvent(QCloseEvent *event);
    private:
      MainWindow (const MainWindow& original);
      MainWindow& operator= (const MainWindow& rhs);
      hexadisplay::ValueProvider::Ptr _value_provider;
      QVBoxLayout* _vbox;
      QLabel* _temperature;
      QLabel* _pressure;
      QLabel* _humidity;
      QLabel* _power;
      QPushButton* _refresh;

  };

};


#endif /* LIBHEXADISPLAY_MAIN_WINDOW_HPP */

