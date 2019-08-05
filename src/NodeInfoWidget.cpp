#include "NodeInfoWidget.hpp"
#include "BagelGui.hpp"

#include <QVBoxLayout>
#include <QPushButton>
#include <QRegExp>

namespace bagel_gui {

  NodeInfoWidget::NodeInfoWidget(mars::cfg_manager::CFGManagerInterface *cfg,
                                 BagelGui *osgBG, QWidget *parent) :
    mars::main_gui::BaseWidget(parent, cfg, "NodeInfoWidget"), osgBG(osgBG) {

    QVBoxLayout *vLayout = new QVBoxLayout();
    dw = new mars::config_map_gui::DataWidget(NULL, 0, true, false);
    vLayout->addWidget(dw);
    setLayout(vLayout);
    std::vector<std::string> editPattern;
    editPattern.push_back("-"); // fake patter to activate check
    dw->setEditPattern(editPattern);
  }

  NodeInfoWidget::~NodeInfoWidget(void) {
  }

  void NodeInfoWidget::setInfo(const configmaps::ConfigMap& map) {
    dw->clearGUI();
    dw->setConfigMap("", map);
  }

  void NodeInfoWidget::clear() {
  }
} // end of namespace bagel_gui
