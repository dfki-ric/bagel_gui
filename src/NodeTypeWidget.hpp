/**
 * \file NodeTypeWidget.hpp
 * \author Malte Langosz
 * \brief 
 **/

#ifndef BAGEL_GUI_NODE_TYPE_WIDGET_HPP
#define BAGEL_GUI_NODE_TYPE_WIDGET_HPP

#ifdef _PRINT_HEADER_
#warning "NodeTypeWidget.hpp"
#endif

#include <mars/main_gui/BaseWidget.h>

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>

namespace bagel_gui {
  class BagelGui;
      
  class NodeTypeWidget : public mars::main_gui::BaseWidget {
    Q_OBJECT
      
  public:
    NodeTypeWidget(mars::cfg_manager::CFGManagerInterface *cfg,
                   BagelGui *osgBG, QWidget *parent = 0);
    ~NodeTypeWidget();
    
    void addNodeType(std::string s);
    void clearNodeTypes();

  public slots:
    void addNode();
    void nodeViewClicked(const QModelIndex &index);
    void nodeViewActivated(const QModelIndex &index);
    void updateFilter(const QString &filter);

  private:
    BagelGui *osgBG;
    QListWidget *nodeTypeView;
    std::vector<std::string> nodeTypes;
    std::string newNode;
    QLineEdit *filterPattern;
  };
  
} // end of namespace bagel_gui

#endif // BAGEL_GUI_NODE_TYPE_WIDGET_HPP
  
