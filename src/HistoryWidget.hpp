/**
 * \file HistoryWidget.hpp
 * \author Malte Langosz
 * \brief 
 **/

#ifndef BAGEL_GUI_HISTORY_WIDGET_HPP
#define BAGEL_GUI_HISTORY_WIDGET_HPP

#include <mars/main_gui/BaseWidget.h>

#include <QWidget>
#include <QListView>
#include <QStringListModel>

namespace bagel_gui {
  class BagelGui;
      
  class HistoryWidget : public mars::main_gui::BaseWidget {
    Q_OBJECT
      
  public:
    HistoryWidget(mars::cfg_manager::CFGManagerInterface *cfg,
		  BagelGui *osgBG, QWidget *parent = 0);
    ~HistoryWidget();
    
    void addHistoryEntry(std::string s);
    void clearHistory() {historyModel->setStringList(QStringList());}

  public slots:
    void historyViewClicked(const QModelIndex &index);

  private:
    BagelGui *osgBG;
    QListView *historyView;
    QStringListModel *historyModel;
  };
  
} // end of namespace bagel_gui

#endif // BAGEL_GUI_HISTORY_WIDGET_HPP
  
