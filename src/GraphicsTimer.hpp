#ifndef BAGEL_GUI_GRAPHICSTIMER_HPP
#define BAGEL_GUI_GRAPHICSTIMER_HPP

#include <QObject>

class QTimerEvent;

namespace bagel_gui {

  class BagelGui;

  class GraphicsTimer : public QObject {
    Q_OBJECT

  public:
    GraphicsTimer(class BagelGui *osgbg);

    void run(int updateTime_ms = 25);
    void stop();
    inline bool isRunning() {return timerId;}

  protected:
    virtual void timerEvent(class QTimerEvent *event);
                        
  private:
    class BagelGui *osgbg;
    int timerId;
  }; // end of class GraphicsTimer

} // end of namespace bagel_gui

#endif /* BAGEL_GUI_GRAPHICSTIMER_HPP */
