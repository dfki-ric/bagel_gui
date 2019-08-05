/**
 * \file SlowWrapper.cpp
 * \author Malte (malte.langosz@dfki.de)
 * \brief A
 *
 * Version 0.1
 */

#include "SlotWrapper.hpp"
#include "BagelGui.hpp"

namespace bagel_gui {

  SlotWrapper::SlotWrapper(BagelGui *bagelGui) : bagelGui(bagelGui) {

  }

  SlotWrapper::~SlotWrapper(void) {
  }

  void SlotWrapper::tabChanged(int index) {
    bagelGui->setTab(index);
  }

  void SlotWrapper::mapChanged() {
    bagelGui->mapChanged();
  }

  void SlotWrapper::closeTab(int index) {
    bagelGui->closeTab(index);
  }

} // end of namespace bagel_gui
