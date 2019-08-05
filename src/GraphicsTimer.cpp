#include "GraphicsTimer.hpp"
#include "BagelGui.hpp"

#include <QDebug>

namespace bagel_gui {

  GraphicsTimer::GraphicsTimer(class BagelGui *_osgbg)
    : osgbg(_osgbg), timerId(0) {}

  void GraphicsTimer::run(int updateTime_ms) {
    if (timerId != 0) {
      qWarning() << "GraphicsTimer is told to run, but is running. stopping";
      stop();
    }
    char* marsTimerStepsize = getenv("MARS_GRAPHICS_UPDATE_TIME");
    if(marsTimerStepsize) {
      updateTime_ms = atoi(marsTimerStepsize);
    }
    // start QObjects own timer, time given in milliseconds. and note the
    // returned id so that we can later compare the id to the actual id of
    // the timeout.
    timerId = startTimer(updateTime_ms);
  }

  void GraphicsTimer::stop() {
    killTimer(timerId);
    timerId = 0;
  }

  void GraphicsTimer::timerEvent(class QTimerEvent *event) {
    if (event->timerId() != timerId) {
      qWarning() << "GraphicsTimer got timerEvent id" << event->timerId()
                 << "but expected" << timerId << "so the event is ignored";
      return;
    }
    osgbg->updateViewer();
  }
 
} // end of namespace bagel_gui
