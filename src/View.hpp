/**
 * \file View.hpp
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief A
 *
 * Version 0.1
 */

#ifndef BAGEL_GUI_VIEW_HPP
#define BAGEL_GUI_VIEW_HPP

// set define if you want to extend the gui

#include <mars/cfg_manager/CFGManagerInterface.h>
#include "NodeLoader.hpp"
#include "ModelInterface.hpp"
#include <string>
#include <osg_graph_viz/View.hpp>
#include <osgViewer/CompositeViewer>
#ifndef Q_MOC_RUN
#include <mars/config_map_gui/DataWidget.h>
#endif

namespace bagel_gui {

  class BagelGui;
  class NodeTypeWidget;
  class HistoryWidget;
  class ForceLayout;

  // inherit from MarsPluginTemplateGUI for extending the gui
  class View : public QObject, public osg_graph_viz::UpdateInterface {
    Q_OBJECT
  public:
    View(BagelGui *m, osg::observer_ptr<osg::GraphicsContext> &shared,
         NodeTypeWidget *ntWidget, HistoryWidget *hWidget,
         mars::config_map_gui::DataWidget *dWidget,
         const std::string &confDir, const std::string &resourcesPath,
         double hFontSize, double pFontSize, double ps, bool classicLook=false);
    ~View();
    QWidget* getWidget() {return viz;}

    osgViewer::View* getOsgView() {return osgView.get();}
    osg_graph_viz::View* getView() {return view.get();}

    void addNode(const std::string &type, std::string name, double x, double y);
    void newEdge(osg_graph_viz::Edge *edge, osg_graph_viz::Node* fromNode,
                 int fromNodeIdx, osg_graph_viz::Node* toNode, int toNodeIdx);
    void nodeSelected(osg_graph_viz::Node* node);
    void edgeSelected(osg_graph_viz::Edge* edge);
    bool removeEdge(osg_graph_viz::Edge* edge);
    bool removeNode(osg_graph_viz::Node* node);
    bool updateNode(osg_graph_viz::Node* node);
    bool updateEdge(osg_graph_viz::Edge* edge);
    void addHistoryEntry();
    void nothingSelected();
    void rightClick(osg_graph_viz::Node *node,
                    int inPort, int outPort,
                    osg_graph_viz::Edge *edge,
                    double x, double y);
    void addHistoryEntry(const std::string &s);
    void loadHistory(size_t index);
    bool groupNodes(const std::string &parent, const std::string &child);

    void updateWidgets();
    void setModel(ModelInterface *m, const std::string &name);
    ModelInterface* getModel() {return model;}
    configmaps::ConfigMap createConfigMap();
    void updateMap(const configmaps::ConfigMap &map);
    void addNode(osg_graph_viz::NodeInfo *info, double x, double y,
                 unsigned long *id, bool onLoad = false, bool reload=false);
    void addEdge(configmaps::ConfigMap edgeMap, bool reload);
    bool hasEdge(configmaps::ConfigMap edgeMap);
    std::string getNodeName(unsigned long id);
    std::string getInPortName(std::string nodeName, unsigned long index);
    std::string getOutPortName(std::string nodeName, unsigned long index);
    osg_graph_viz::NodeInfo getNodeInfo(const std::string &name);
    void cloneNodeToView(configmaps::ConfigMap node);
    void clearGraph();
    void undo() override;
    void redo() override;
    void updateNodeMap(const std::string &nodeName,
                       const configmaps::ConfigMap &map);
    void updateEdgeMap(const std::string &edgeName, const configmaps::ConfigMap &map);

    const configmaps::ConfigMap* getNodeMap(const std::string &nodeName);
    const configmaps::ConfigMap *getEdgeMap(const std::string &edgeName);

    void forceDirectedLayoutStep();
    void setUseForceLayout(bool v) {useForceLayout = v;}
    bool getUseForceLayout() {return useForceLayout;}
    void loadLayout(const std::string&);
    void saveLayout(const std::string&);
    void saveLayout();
    void restoreLayout();
    configmaps::ConfigMap getLayout();
    void applyLayout(configmaps::ConfigMap &layout);
    
    bool hasChanges() const {return history.size() > 0;}
    

    osg_graph_viz::Node* addNode(configmaps::ConfigMap node);
    configmaps::ConfigMap getTypeInfo(const std::string &nodeType);
    void removeNode(const std::string &name);

  public slots:
    void contextRemoveNode();
    void contextRemoveEdge();
    void toggleInputInterface();
    void toggleOutputInterface();
    void nodeContextClicked();
    void edgeContextClicked();
    void inPortContextClicked();
    void outPortContextClicked();
    void contextDecoupleEdge();

  private:
    BagelGui *mainLib;
    QWidget *viz;
    osg::ref_ptr<osg_graph_viz::View> view;
    osg::ref_ptr<osgViewer::View> osgView;
    osg::ref_ptr<osg_graph_viz::Node> contextNode;
    int contextPort;
    osg::ref_ptr<osg_graph_viz::Edge> contextEdge;
    ModelInterface *model;
    NodeTypeWidget *ntWidget;
    HistoryWidget *hWidget;
    mars::config_map_gui::DataWidget *dWidget;
    std::string confDir, resourcesPath, modelName;
    unsigned long lastAdd;
    ssize_t historyIndex = -1;

    // node info container
    std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> > nodeMap;
    std::map<osg_graph_viz::Node*, unsigned long> nodeIdMap;

    //std::map<osg_graph_viz::Node*, configmaps::ConfigMap> nodeConfigMap;
    std::list<osg::ref_ptr<osg_graph_viz::Edge> > edgeList;
    std::map<std::string, osg_graph_viz::NodeInfo> infoMap;
    unsigned long nextNodeId, nextOrderNumber, updateNodeId, nextEdgeId;
    std::vector<configmaps::ConfigMap> history;
    std::vector<std::string> historyNames;
    ForceLayout *layout;
    bool useForceLayout;
    configmaps::ConfigMap currentLayout;

    std::string handleNodeName(std::string name, std::string type);
    osg::ref_ptr<osg_graph_viz::Node> getNodeByName(const std::string&);
    osg::ref_ptr<osg_graph_viz::Edge> getEdgeByName(const std::string &);
    unsigned long getNodeId(const std::string &name);
    /*
     * Since we are saving history before we remove a node, when we click a history item to be applied
     * from the history widget, it must clear the graph to load the history state, the problem is
     * clearGraph() calls again the removeNode(), which also saves a history entry which we don't want to do when clearing graph.
     * So this flag tells us if graph is being cleared or not so we don't save history when clearing the graph
     */
    bool clearing_graph{false};

  }; // end of class definition View

} // end of namespace bagel_bui

#endif // BAGEL_GUI_VIEW_HPP