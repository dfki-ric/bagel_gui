/**
 * \file SlowWrapper.hpp
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief A
 *
 * Version 0.1
 */

#ifndef BAGEL_SLOT_WRAPPER_HPP
#define BAGEL_SLOT_WRAPPER_HPP


#include <QObject>

namespace bagel_gui {
  class BagelGui;

  class SlotWrapper : public QObject {
    Q_OBJECT

  public:
    SlotWrapper(BagelGui *bagelGui);
    ~SlotWrapper();

  public slots:
    void tabChanged(int);
    void mapChanged();
    void closeTab(int);

  private:
    BagelGui *bagelGui;
  };

} // end of namespace bagel_gui

#endif // BAGEL_GUI_SLOT_WRAPPER_HPP
