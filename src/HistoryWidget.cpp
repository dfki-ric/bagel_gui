#include "HistoryWidget.hpp"
#include "BagelGui.hpp"

#include <QVBoxLayout>
#include <QPushButton>

namespace bagel_gui {

  HistoryWidget::HistoryWidget(mars::cfg_manager::CFGManagerInterface *cfg,
			       BagelGui *osgBG, QWidget *parent) :
    mars::main_gui::BaseWidget(parent, cfg, "HistoryWidget"), osgBG(osgBG) {

    QVBoxLayout *vLayout = new QVBoxLayout();

    historyView = new QListView();
    historyModel = new QStringListModel();
    historyView->setModel(historyModel);
    connect(historyView, SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(historyViewClicked(const QModelIndex&)));
    vLayout->addWidget(historyView);
    setLayout(vLayout);

    historyView->setSelectionMode(QAbstractItemView::SingleSelection);
  }

  HistoryWidget::~HistoryWidget(void) {
  }

  void HistoryWidget::addHistoryEntry(std::string s) {
    historyModel->insertRow(historyModel->rowCount());
    QModelIndex index = historyModel->index(historyModel->rowCount()-1);
    historyModel->setData(index, QString(s.c_str()));
  }

  void HistoryWidget::historyViewClicked(const QModelIndex &index) {
    osgBG->loadHistory(index.row());
  }

} // end of namespace bagel_gui
