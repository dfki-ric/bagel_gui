#include "NodeTypeWidget.hpp"
#include "BagelGui.hpp"

#include <QVBoxLayout>
#include <QPushButton>
#include <QRegExp>

namespace bagel_gui {

  NodeTypeWidget::NodeTypeWidget(mars::cfg_manager::CFGManagerInterface *cfg,
                                 BagelGui *osgBG, QWidget *parent) :
    mars::main_gui::BaseWidget(parent, cfg, "NodeTypeWidget"), osgBG(osgBG) {

    QVBoxLayout *vLayout = new QVBoxLayout();

    //QPushButton *button = new QPushButton("add node", this);
    //connect(button, SIGNAL(clicked()), this, SLOT(addNode()));
    filterPattern = new QLineEdit();
    connect(filterPattern, SIGNAL(textChanged(const QString&)),
            this, SLOT(updateFilter(const QString&)));
    vLayout->addWidget(filterPattern);

    nodeTypeView = new QListWidget();
    connect(nodeTypeView,
            SIGNAL(clicked(const QModelIndex&)),
            this, SLOT(nodeViewClicked(const QModelIndex&)));
    connect(nodeTypeView,
            SIGNAL(activated(const QModelIndex&)),
            this, SLOT(nodeViewActivated(const QModelIndex&)));
    vLayout->addWidget(nodeTypeView);
    setLayout(vLayout);

    //nodeTypeView->setSelectionMode(QAbstractItemView::SingleSelection);
  }

  NodeTypeWidget::~NodeTypeWidget(void) {
  }

  void NodeTypeWidget::addNodeType(std::string s) {
    //xnodeTypeModel->insertRow(nodeTypeModel->rowCount());
    //QModelIndex index = nodeTypeModel->index(nodeTypeModel->rowCount()-1);
    //nodeTypeModel->setData(index, QString(s.c_str()));
    nodeTypes.push_back(s);
    updateFilter(filterPattern->text());
  }

  void NodeTypeWidget::nodeViewClicked(const QModelIndex &index) {
    QVariant v = nodeTypeView->model()->data(index, 0);
    if(v.isValid()) {
      newNode = v.toString().toStdString();
      osgBG->nodeTypeSelected(newNode);
    }
  }

  void NodeTypeWidget::nodeViewActivated(const QModelIndex &index) {
    QVariant v = nodeTypeView->model()->data(index, 0);
    if(v.isValid()) {
      newNode = v.toString().toStdString();
    }
    addNode();
  }

  void NodeTypeWidget::addNode() {
    if(!newNode.empty()) {
      osgBG->addNode(newNode);
    }
  }

  void NodeTypeWidget::clearNodeTypes() {
    nodeTypes.clear();
    nodeTypeView->clear();
  }

  void NodeTypeWidget::updateFilter(const QString &filter) {
    std::vector<std::string>::iterator it = nodeTypes.begin();
    QRegExp exp(filter);

    nodeTypeView->clear();
    for(; it!=nodeTypes.end(); ++it) {
      if(exp.indexIn(it->c_str()) != -1) {
        nodeTypeView->addItem(it->c_str());
      }
    }
  }
} // end of namespace bagel_gui
