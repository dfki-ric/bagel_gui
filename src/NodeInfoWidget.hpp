/**
 * \file NodeInfoWidget.hpp
 * \author Malte Langosz
 * \brief
 **/

#ifndef BAGEL_GUI_NODE_INFO_WIDGET_HPP
#define BAGEL_GUI_NODE_INFO_WIDGET_HPP

#ifdef _PRINT_HEADER_
#warning "NodeInfoWidget.hpp"
#endif

#include <main_gui/BaseWidget.h>
#include <configmaps/ConfigMap.hpp>

#include <QWidget>

namespace mars {
  namespace config_map_gui {
    class DataWidget;
  }
}

namespace bagel_gui {
  class BagelGui;

  class NodeInfoWidget : public mars::main_gui::BaseWidget {
    Q_OBJECT

  public:
    NodeInfoWidget(mars::cfg_manager::CFGManagerInterface *cfg,
                   BagelGui *osgBG, QWidget *parent = 0);
    ~NodeInfoWidget();

    void setInfo(const configmaps::ConfigMap &map);
    void clear();

  private:
    BagelGui *osgBG;
    mars::config_map_gui::DataWidget *dw;
  };

} // end of namespace bagel_gui

#endif // BAGEL_GUI_NODE_INFO_WIDGET_HPP
