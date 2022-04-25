/**
 * \file View.cpp
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief A
 */


#include "GraphicsTimer.hpp"
#include "View.hpp"
#include "BagelGui.hpp"
#include "BagelLoader.hpp"
#include "BagelModel.hpp"
#include "NodeTypeWidget.hpp"
#include "HistoryWidget.hpp"
#include "ForceLayout.hpp"

#include <mars/utils/misc.h>

#include <QHBoxLayout>
#include <QSplitter>
#include <QMenu>
#include <QAction>
#include <QPoint>

#include <osgViewer/View>

#include <osgQt/GraphicsWindowQt>
#include <sstream>

#include <assert.h>
#include <dirent.h>         /* directory search */

extern "C" {
  typedef void (*node_info_t) (char **name, size_t *num_inputs,
                               size_t *num_outpus);
}

namespace bagel_gui {

  using namespace configmaps;

  View::View(BagelGui *m, osg::observer_ptr<osg::GraphicsContext> &shared,
             NodeTypeWidget* ntWidget,HistoryWidget* hWidget,
             mars::config_map_gui::DataWidget* dWidget,
             const std::string &confDir,
             const std::string &resourcesPath,
             double hFontSize, double pFontSize,
             double ps, bool classicLook ) : mainLib(m), ntWidget(ntWidget),
         hWidget(hWidget),
         dWidget(dWidget),
         confDir(confDir),
         resourcesPath(resourcesPath),
         layout(new ForceLayout(nodeMap, nodeIdMap, edgeList)) {
    lastAdd = 0;
    updateNodeId = 0;
    useForceLayout = false;
    model = NULL;
    view = new osg_graph_viz::View();
    if(resourcesPath != "") {
      view->setResourcesPath(resourcesPath + "/osg_graph_viz/");
    }
    view->init(hFontSize, pFontSize, ps, classicLook);
    view->setUpdateInterface(this);
    osgView = new osgViewer::View();
    osg::Camera *camera = new osg::Camera();

    osg::DisplaySettings* ds = osg::DisplaySettings::instance();
    osg::ref_ptr<osg::GraphicsContext::Traits> traits;
    traits = new osg::GraphicsContext::Traits;
    double w = 1920;
    double h = 1080;
    traits->readDISPLAY();
    if (traits->displayNum < 0)
      traits->displayNum = 0;
    traits->x = 100;
    traits->y = 100;
    traits->width = w;
    traits->height = h;
    traits->windowDecoration = true;
    traits->doubleBuffer = true;
    traits->alpha = ds->getMinimumNumAlphaBits();
    traits->stencil = ds->getMinimumNumStencilBits();
    traits->sampleBuffers = ds->getMultiSamples();
    traits->samples = ds->getNumMultiSamples();
    traits->vsync = false;
    traits->sharedContext = shared;
    osgQt::GraphicsWindowQt *gw2 = new osgQt::GraphicsWindowQt(traits.get());
    camera->setGraphicsContext(gw2);
    if(!shared.valid()) shared = gw2;
    camera->setViewport( new ::osg::Viewport(0, 0, w, h) );
    camera->setProjectionMatrix(osg::Matrix::ortho2D(0,w,0,h));
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setViewMatrix(osg::Matrix::identity());
    //camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    camera->setRenderOrder(osg::Camera::POST_RENDER, 10);
    camera->setClearColor(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    osg::StateSet *s = camera->getOrCreateStateSet();
    s->setMode(GL_LIGHTING, (osg::StateAttribute::OFF |
                             osg::StateAttribute::OVERRIDE |
                             osg::StateAttribute::PROTECTED));
    s->setMode(GL_DEPTH_TEST, (osg::StateAttribute::OFF |
                               osg::StateAttribute::OVERRIDE |
                               osg::StateAttribute::PROTECTED));
    s->setRenderingHint(osg::StateSet::TRANSPARENT_BIN |
                        osg::StateAttribute::OVERRIDE |
                        osg::StateAttribute::PROTECTED);
    s->setMode(GL_BLEND,osg::StateAttribute::ON |
               osg::StateAttribute::OVERRIDE |
               osg::StateAttribute::PROTECTED);
    view->setCamera(camera);
    osgView->setCamera(camera);
    osg::Group *g = new osg::Group();
    g->getOrCreateStateSet()->setGlobalDefaults();
    g->addChild(view->getScene());
    osgView->setSceneData(g);

    osgQt::GLWidget *widget1 = gw2->getGLWidget();
    widget1->setGeometry(100, 100, 1920, 1080);
    viz = dynamic_cast<QWidget*>(widget1);
    osgView->addEventHandler(view.get());

    nextNodeId = nextEdgeId = nextOrderNumber = 1;
  }

  View::~View() {
    if(model) delete model;
    delete layout;
  }

  void View::setModel(ModelInterface *m, const std::string &name) {
    modelName = name;
    if(model) delete model;
    model = m;
    updateWidgets();
  }

  void View::updateWidgets() {
    if(model) {
      infoMap = model->getNodeInfoMap();
    }
    ntWidget->clearNodeTypes();
    {
      std::map<std::string, osg_graph_viz::NodeInfo>::iterator it=infoMap.begin();
      for(; it!=infoMap.end(); ++it) {
        ntWidget->addNodeType(it->first);
      }
    }
    {
      hWidget->clearHistory();
      std::vector<std::string>::iterator it=historyNames.begin();
      for(; it!=historyNames.end(); ++it) {
        hWidget->addHistoryEntry(*it);
      }
    }
  }

  void View::nothingSelected() {
    dWidget->clearGUI();
  }

  void View::addHistoryEntry() {
    char s[55];
    sprintf(s, "history: %lu", history.size()+1);
    addHistoryEntry(s);
  }

  void View::addHistoryEntry(const std::string &s) {
    history.push_back(createConfigMap());
    historyNames.push_back(s);
    hWidget->addHistoryEntry(s);
  }

  void View::loadHistory(size_t index) {
    mainLib->load(history[index], true);
  }

  void View::nodeSelected(osg_graph_viz::Node* node) {
    updateNodeId = nodeIdMap[node];
    dWidget->setConfigMap("", node->getMap());
  }

  void View::edgeSelected(osg_graph_viz::Edge* edge) {
    updateNodeId = 0;
    dWidget->setConfigMap("", edge->getMap());
  }

  bool View::removeEdge(osg_graph_viz::Edge* edge) {
    ConfigMap map = edge->getMap();
    if(!model->removeEdge(map["id"])) return false;

    std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it;
    for(it=edgeList.begin(); it!=edgeList.end(); ++it) {
      if(*it == edge) {
        // todo: call removeEdge form ModelInterface
        edgeList.erase(it);
        break;
      }
    }
    if(contextEdge.get() == edge) {
      contextEdge = NULL;
    }
    return true;
  }

  bool View::removeNode(osg_graph_viz::Node* node) {
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    if(nodeIdMap.find(node) != nodeIdMap.end()) {
      unsigned long id = nodeIdMap[node];

      if(!model->removeNode(id)) return false;
      nodeIdMap.erase(node);
      nodeMap.erase(id);
    }
    if(contextNode.get() == node) {
      contextNode = NULL;
    }
    return true;
  }

  // update the gui
  bool View::updateNode(osg_graph_viz::Node* node) {
    if(nodeIdMap.find(node) != nodeIdMap.end()) {
      updateNodeId = nodeIdMap[node];
      dWidget->updateConfigMap("", node->getMap());
    }
    return true;
  }

  // update the gui
  bool View::updateEdge(osg_graph_viz::Edge* edge) {
    updateNodeId = 0;
    dWidget->updateConfigMap("", edge->getMap());
    return true;
  }

  // This method is called if a new edge is added in the gui
  void View::newEdge(osg_graph_viz::Edge *edge, osg_graph_viz::Node* fromNode,
                     int fromNodeIdx, osg_graph_viz::Node* toNode,
                     int toNodeIdx) {
    ConfigMap edgeConfig = edge->getMap();
    ConfigMap fromNodeMap = fromNode->getMap();
    ConfigMap toNodeMap = toNode->getMap();
    bool valid = true;
    if((std::string)fromNodeMap["name"] == "" ||
       (std::string)toNodeMap["name"] == "") {
      valid = false;
    }
    if((std::string)fromNodeMap["outputs"][fromNodeIdx]["name"] == "" ||
       (std::string)toNodeMap["inputs"][toNodeIdx]["name"] == "") {
      valid = false;
    }
    if(!valid) {
      view->removeEdge(edge);
    }
    else {
      edgeConfig["fromNode"] = fromNodeMap["name"];
      edgeConfig["fromNodeOutput"] = fromNodeMap["outputs"][fromNodeIdx]["name"];

      edgeConfig["toNode"] = toNodeMap["name"];
      edgeConfig["toNodeInput"] = toNodeMap["inputs"][toNodeIdx]["name"];
      edgeConfig["weight"] = 1.0;
      edgeConfig["ignore_for_sort"] = 0;
      if(model->addEdge(nextEdgeId, &edgeConfig)) {
        edgeConfig["id"] = nextEdgeId++;
        edge->updateMap(edgeConfig);
        edgeList.push_back(edge);
      }
      else {
        view->removeEdge(edge);
      }
    }
  }

  // This method is called on gui respone to add a new node
  void View::addNode(const std::string &type, std::string name,
                     double x, double y) {
    // check if we have the type in the list
    if(infoMap.find(type) == infoMap.end()) {
      fprintf(stderr, "could not add node because type '%s' unknown\n", type.c_str());
      return;
    }
    osg_graph_viz::NodeInfo info = infoMap[type];
    info.map["name"] = name = handleNodeName(name, type);
    fprintf(stderr, "add node: %s %lu\n", name.c_str(), nextNodeId);
    info.map["id"] = nextNodeId;
    // check wether adding node is valid in model
    if(!model->addNode(nextNodeId, &(info.map))) {
      return;
    }
    // todo: verify that the name is still unique?

    info.map["order"] = nextOrderNumber++;
    osg_graph_viz::Node *node = view->createNode(info);
    if(x == 0.0 && y == 0.0){
      view->getPosition(&x, &y);
    }
    if(currentLayout.hasKey(name)) {
      x = currentLayout[name]["x"];
      y = currentLayout[name]["y"];
    }
    node->setAbsolutePosition(x, y);
    nodeMap[nextNodeId] = node;
    nodeIdMap[node] = nextNodeId;
    lastAdd = nextNodeId;
    model->preAddNode(nextNodeId++);
    //fprintf(stderr, "added node '%s'\n", string(info.map["name"]).c_str());
  }

  std::string View::handleNodeName(std::string name, std::string type) {
    std::string newName = name;
    { // generate name if not given
      if(newName == "") {
        int i=1;
        char buffer[50];
        sprintf(buffer, "%d", i);
        newName = type + buffer;
        while(getNodeByName(newName).valid()) {
          sprintf(buffer, "%d", ++i);
          newName = type + buffer;
        }
      }
    }

    { // check if the name is not taken, otherise add counter to name
      unsigned long cnt = 1;
      std::string name_ = newName;
      if(newName[newName.size()-4] == '_' &&
         isdigit(newName[newName.size()-3]) &&
         isdigit(newName[newName.size()-2]) &&
         isdigit(newName[newName.size()-1])) {
        newName = newName.substr(0, newName.size()-4);
      }
      while(getNodeByName(name_).valid()) {
        char buffer[50];
        sprintf(buffer, "_%03lu", cnt);
        name_ = newName+buffer;
        ++cnt;
      }
      newName = name_;
    }
    return newName;
  }

  void View::updateMap(const ConfigMap &map) {
    if(updateNodeId) {
      if(!model->updateNode(updateNodeId, map)) {
        return;
      }
    }
    else {
      if(!model->updateEdge(0, map)) {
        return;
      }
    }
    view->updateMap(map);
  }

  void View::updateNodeMap(const std::string &nodeName, const ConfigMap &map) {
    osg::ref_ptr<osg_graph_viz::Node> node = getNodeByName(nodeName);
    if(!node.valid()) {
      fprintf(stderr, "ERROR: updateNodeMap cannot find node by name: %s!\n", nodeName.c_str());
      return;
    }
    unsigned long nodeId = nodeIdMap[node.get()];
    if(!model->updateNode(nodeId, map)) {
      return;
    }
    node->updateMap(map);
    if(nodeId == updateNodeId) {
      dWidget->updateConfigMap("", node->getMap());
    }
  }

  const configmaps::ConfigMap* View::getNodeMap(const std::string &nodeName) {
    osg::ref_ptr<osg_graph_viz::Node> node = getNodeByName(nodeName);
    if(!node.valid()) {
      fprintf(stderr, "ERROR: getNodeMap cannot find node by name: %s!\n", nodeName.c_str());
      return NULL;
    }
    return &(node->getMap());
  }

  // This method is called from loading or import functionality
  void View::addNode(osg_graph_viz::NodeInfo *info, double x, double y,
                     unsigned long *id, bool onLoad, bool reload) {
    if(*id >= nextNodeId) {
      nextNodeId = *id+1;
    }
    else if(*id==0) {
      *id = nextNodeId++;
    }
    lastAdd = *id;
    std::string name;
    if(info->map.hasKey("order")) {
      unsigned long order = info->map["order"];
      if(order >= nextOrderNumber) nextOrderNumber = order+1;
    }
    if(info->map.hasKey("name")) name << info->map["name"];
    info->map["name"] = name = handleNodeName(name, info->map["type"]);
    // check if node is valid in model
    if(!model->addNode(*id, info->map)) {
      return;
    }
    // todo: verify that the node name is still unique
    // create the viz node
    osg_graph_viz::Node *node = view->createNode(*info);
    if(currentLayout.hasKey(name)) {
      if(reload) {
        currentLayout[name]["x"] = x;
        currentLayout[name]["y"] = y;
      } else {
        x = currentLayout[name]["x"];
        y = currentLayout[name]["y"];
      }
    }
    node->setPosition(x, y);
    nodeMap[*id] = node;
    nodeIdMap[node] = *id;
    // handle node group
    if(info->map.hasKey("parentName") && !info->map["parentName"].getString().empty()) {
      if(!model->groupNodes(getNodeId(info->map["parentName"].getString()), *id)) {
        // ungroup in bagel gui
        fprintf(stderr, "ungroup node: %s\n", info->map["name"].getString().c_str());
        info->map["parentName"] = "";
        node->updateMap(info->map);
      }
    }
    if(!onLoad) {
      model->preAddNode(*id);
    }
  }

  std::string View::getNodeName(unsigned long id) {
    if(nodeMap.find(id) != nodeMap.end()) {
      return nodeMap[id]->getName();
    }
    return std::string();
  }

  std::string View::getInPortName(std::string nodeName, unsigned long index) {
    osg::ref_ptr<osg_graph_viz::Node> node = getNodeByName(nodeName);
    if(node.valid()) {
      return node->getInPortName(index);
    }
    return std::string();
  }

  std::string View::getOutPortName(std::string nodeName, unsigned long index) {
    osg::ref_ptr<osg_graph_viz::Node> node = getNodeByName(nodeName);
    if(node.valid()) {
      return node->getOutPortName(index);
    }
    return std::string();
  }

  osg_graph_viz::NodeInfo View::getNodeInfo(const std::string &name) {
    if(infoMap.find(name) == infoMap.end()) {
      fprintf(stderr, "ERROR: was not able to find node information for: %s\n", name.c_str());
      return osg_graph_viz::NodeInfo();
    }
    return infoMap[name];
  }

  // This method is used form import or loader functions
  // Reload means this method is called from history event or view switch
  // view switch should be handled differently maybe history to
  void View::addEdge(ConfigMap edgeMap, bool reload) {
    unsigned long id1;
    unsigned long idx1;
    unsigned long id2;
    unsigned long idx2;
    osg_graph_viz::Node *fromNode, *toNode;

    // assure the keys that we need
    if(!edgeMap.hasKey("fromNode") || !edgeMap.hasKey("toNode") ||
       !edgeMap.hasKey("fromNodeOutput") || !edgeMap.hasKey("toNodeInput")) {
      fprintf(stderr, "ERROR: invalid edge information for adding edge");
      return;
    }

    // now we have all the information in the map to add the edge in the model
    if(reload) {
      if(!model->addEdge(nextEdgeId, edgeMap)) {
        return;
      }
    }
    else {
      if(!model->addEdge(nextEdgeId, &edgeMap)) {
        return;
      }
    }

    edgeMap["id"] = nextEdgeId++;
    // todo: most of this code should move to the bagelloader
    // check the starting node and port
    fromNode = getNodeByName(edgeMap["fromNode"]);
    id1 = nodeIdMap[fromNode];
    if(!fromNode) {
      fprintf(stderr, "addEdge:  edge ignored; searching for fromNode:\n%s\n",
              edgeMap.toYamlString().c_str());
      return;
    }

    {
      ConfigMap m = fromNode->getMap();
      std::string portName = edgeMap["fromNodeOutput"];
      idx1 = 0;
      bool found = false;
      if(m.hasKey("outputs")) {
        for(size_t i=0; i<m["outputs"].size(); ++i) {
          if((std::string)m["outputs"][i]["name"] == portName) {
            found = true;
            idx1 = i;
            break;
          }
        }
      }
      if(!found) {
        fprintf(stderr, "WARNING: edge ignored; could not find fromNode port:\n%s\n",
                edgeMap.toYamlString().c_str());
        return;
      }
    }

    // check the ending node and port
    toNode = getNodeByName(edgeMap["toNode"]);
    id2 = nodeIdMap[toNode];
    idx2 = 0;
    if(!toNode) {
      fprintf(stderr, "addEdge: edge ignored; searching for toNode:\n%s\n",
              edgeMap.toYamlString().c_str());
      return;
    }

    if(edgeMap.hasKey("toNodeInput")) {
      ConfigMap m = toNode->getMap();
      std::string portName = edgeMap["toNodeInput"];
      bool found = false;
      if(m.hasKey("inputs")) {
        for(size_t i=0; i<m["inputs"].size(); ++i) {
          if((std::string)m["inputs"][i]["name"] == portName) {
            found = true;
            idx2 = i;
            break;
          }
        }
      }
      if(!found) {
        fprintf(stderr, "WARNING: edge ignored; could not find toNode port:\n%s\n",
                edgeMap.toYamlString().c_str());
        return;
      }
    }

    // todo: the vertices handling should move into the view library
    osg::Vec3 outV = fromNode->getOutPortPos(idx1);
    osg::Vec3 inV = toNode->getInPortPos(idx2);
    double startOffset = 0, endOffset = 0;
    if(edgeMap.hasKey("vertices") && !fromNode->getNodeInfo().redrawEdges && !toNode->getNodeInfo().redrawEdges) {
      startOffset = edgeMap["vertices"][0]["y"];
      startOffset -= outV.y();
      endOffset = edgeMap["vertices"][edgeMap["vertices"].size()-1]["y"];
      endOffset -= inV.y();
    }
    else {
      size_t last = 1;
      if(edgeMap.hasKey("vertices")) {
        last = edgeMap["vertices"].size() - 1;
      }
      edgeMap["vertices"][0]["x"] = outV.x();
      edgeMap["vertices"][0]["y"] = outV.y();
      edgeMap["vertices"][0]["z"] = outV.z();

      edgeMap["vertices"][last]["x"] = inV.x();
      edgeMap["vertices"][last]["y"] = inV.y();
      edgeMap["vertices"][last]["z"] = inV.z();
    }
    osg_graph_viz::Edge *edge = view->createEdge(edgeMap, idx1, idx2);
    edge->setStartOffset(startOffset);
    edge->setEndOffset(endOffset);
    nodeMap[id1]->addOutputEdge(idx1, edge);
    nodeMap[id2]->addInputEdge(idx2, edge);
    edgeList.push_back(edge);
  }

  bool View::hasEdge(ConfigMap edgeMap) {
    // assure the keys that we need
    if(!edgeMap.hasKey("fromNode") || !edgeMap.hasKey("toNode") ||
       !edgeMap.hasKey("fromNodeOutput") || !edgeMap.hasKey("toNodeInput")) {
      fprintf(stderr, "ERROR: invalid edge information to search existing edge; returning not found");
      return false;
    }
    return model->hasEdge(edgeMap);
  }

  ConfigMap View::createConfigMap() {
    ConfigMap conf;
    std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    int i=0;
    conf["model"] = modelName;
    BagelModel *bm = dynamic_cast<BagelModel*>(model);
    if(bm) {
      std::string path = bm->getExternNodePath();
      if(!path.empty()) {
        conf["externNodePath"] = path;
      }
    }
    for(it=nodeMap.begin(); it!=nodeMap.end(); ++it, ++i) {
      ConfigMap map = it->second->getMap();

      // filter out descritpions
      if((std::string)map["type"] == "DES"){
        conf["descriptions"] += map;
      }
      else if((std::string)map["type"] == "META"){
        conf["meta"] += map;
      }
      else {
        conf["nodes"] += map;
      }
    }

    std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator eit;
    for(eit=edgeList.begin(); eit!=edgeList.end(); ++eit) {
      ConfigMap map = (*eit)->getMap();
      conf["edges"] += map;
    }
    return conf;
  }

  void View::clearGraph() {
    size_t t;
    nextNodeId = nextEdgeId = nextOrderNumber = 1;
    while(nodeMap.begin() != nodeMap.end()) {
      t = nodeMap.size();
      view->removeNode(nodeMap.begin()->second.get());
      if(t == nodeMap.size()) {
        // remove failed
        fprintf(stderr, "ERROR: clear Graph failed!");
        return;
      }
    }
  }

  bool View::groupNodes(const std::string &parent, const std::string &child) {
    unsigned long groupId=0, nodeId;
    osg::ref_ptr<osg_graph_viz::Node> node;
    if(!parent.empty()) {
      node = getNodeByName(parent);
      if(!node.valid()) {
        fprintf(stderr, "View::groupNodes: parent %s not found\n",
                parent.c_str());
        return false;
      }
      groupId = nodeIdMap[node.get()];
    }
    node = getNodeByName(child);
    if(!node.valid()) {
      fprintf(stderr, "View::groupNodes: child %s not found\n",
              parent.c_str());
      return false;
    }
    nodeId = nodeIdMap[getNodeByName(child)];
    return model->groupNodes(groupId, nodeId);
  }

  void View::forceDirectedLayoutStep()
  {
    if(useForceLayout) layout->step();
  }

  osg::ref_ptr<osg_graph_viz::Node> View::getNodeByName(const std::string &name) {
    std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    for(it=nodeMap.begin(); it!=nodeMap.end(); ++it) {
      if(it->second->getName() == name) {
        return it->second;
      }
    }
    return NULL;
  }

  unsigned long View::getNodeId(const std::string &name) {
    std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    for(it=nodeMap.begin(); it!=nodeMap.end(); ++it) {
      if(it->second->getName() == name) {
        return it->first;
      }
    }
    return 0;
  }

  void View::loadLayout(const std::string &filename) {
    // todo: update layout with node move events
    if(mars::utils::pathExists(filename)) {
      currentLayout = ConfigMap::fromYamlFile(filename);
      restoreLayout();
    }
  }

  ConfigMap View::getLayout() {
    saveLayout();
    return currentLayout;
  }

  void View::applyLayout(ConfigMap &layout) {
    currentLayout = layout;
    restoreLayout();
  }

  void View::saveLayout(const std::string &filename) {
    saveLayout();
    currentLayout.toYamlFile(filename);
  }

  void View::saveLayout() {
    double x, y, s;
    std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    currentLayout.clear();
    for(it=nodeMap.begin(); it!=nodeMap.end(); ++it) {
      it->second->getPosition(&x, &y);
      currentLayout[it->second->getName()]["x"] = x;
      currentLayout[it->second->getName()]["y"] = y;
    }
    view->getViewPos(&x, &y);
    view->getViewScale(&s);
    currentLayout["view"]["scale"] = s;
    currentLayout["view"]["x"] = x;
    currentLayout["view"]["y"] = y;
  }

  void View::restoreLayout() {
    double x, y, s;
    std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    osg::ref_ptr<osg_graph_viz::Node> parent;
    std::vector<osg_graph_viz::Node *> childs;

    for(it=nodeMap.begin(); it!=nodeMap.end(); ++it) {
      parent = it->second->getParentNode();
      if(!parent) {
        const std::string &name = it->second->getName();
        if(currentLayout.hasKey(name)) {
          x = currentLayout[name]["x"];
          y = currentLayout[name]["y"];
          it->second->setAbsolutePosition(x, y);
        }
      }
      else {
        childs.push_back(it->second.get());
      }
    }
    std::vector<osg_graph_viz::Node*>::iterator childIt = childs.begin();
    for(; childIt!=childs.end(); ++childIt) {
      const std::string &name = (*childIt)->getName();
      if(currentLayout.hasKey(name)) {
        x = currentLayout[name]["x"];
        y = currentLayout[name]["y"];
        (*childIt)->setAbsolutePosition(x, y);
      }
    }
    if(currentLayout.hasKey("view")) {
      s = currentLayout["view"]["scale"];
      x = currentLayout["view"]["x"];
      y = currentLayout["view"]["y"];
      view->scaleView(s);
      view->setViewPos(x, y);
    }
  }

  osg_graph_viz::Node* View::addNode(ConfigMap node) {
    std::string type = node["type"];
    std::string name = node["name"];
    // handle subgraph and extern types
    if(type == "SUBGRAPH") {
      type << node["subgraph_name"];
    }
    else if(type == "EXTERN") {
      type << node["extern_name"];
    }
    std::size_t found = name.find(type);
    if (found!=std::string::npos) {
      name = "";
    }
    addNode(type, name, 0, 0);
    return nodeMap[lastAdd];
  }

  configmaps::ConfigMap View::getTypeInfo(const std::string &nodeType) {
    if(infoMap.find(nodeType) == infoMap.end()) {
      return ConfigMap();
    }
    return infoMap[nodeType].map;
  }

  void View::rightClick(osg_graph_viz::Node *node,
                        int inPort, int outPort,
                        osg_graph_viz::Edge *edge,
                        double x, double y) {
    if(inPort >= 0) {
      QMenu contextMenu("Context menu");
      //fprintf(stderr, "node %d\n", node);

      contextNode = node;
      contextPort = inPort;
      QAction action1("toggle interface", this);
      connect(&action1, SIGNAL(triggered()), this, SLOT(toggleInputInterface()));
      contextMenu.addAction(&action1);
      QAction action2("delete node", this);
      connect(&action2, SIGNAL(triggered()), this, SLOT(contextRemoveNode()));
      contextMenu.addAction(&action2);
      std::vector<std::string> cStrings = mainLib->getInPortContextStrings(node->getName(), node->getInPortName(inPort));
      std::vector<QAction*> actions;
      for(auto label: cStrings) {
        QAction *action = new QAction(label.c_str(), this);
        connect(action, SIGNAL(triggered()), this, SLOT(inPortContextClicked()));
        contextMenu.addAction(action);
      }

      // todo convert to correct position
      contextMenu.exec(viz->mapToGlobal(QPoint(x*viz->width(),
                                               (1-y)*viz->height())));
      for(auto action: actions) {
        delete action;
      }
      return;
    }
    if(outPort >= 0) {
      QMenu contextMenu("Context menu");
      //fprintf(stderr, "node %d\n", node);

      contextNode = node;
      contextPort = outPort;
      QAction action1("toggle interface", this);
      connect(&action1, SIGNAL(triggered()), this, SLOT(toggleOutputInterface()));
      contextMenu.addAction(&action1);
      /*//It's really frustrating, if you have gross sensory motor skills and just want to toggle or to configure
      QAction action2("delete node", this);
      connect(&action2, SIGNAL(triggered()), this, SLOT(contextRemoveNode()));
      contextMenu.addAction(&action2);
      */
      std::vector<std::string> cStrings = mainLib->getOutPortContextStrings(node->getName(), node->getOutPortName(outPort));
      std::vector<QAction*> actions;
      for(auto label: cStrings) {
        QAction *action = new QAction(label.c_str(), this);
        connect(action, SIGNAL(triggered()), this, SLOT(outPortContextClicked()));
        contextMenu.addAction(action);
      }
      // todo convert to correct position
      contextMenu.exec(viz->mapToGlobal(QPoint(x*viz->width(),
                                               (1-y)*viz->height())));
      for(auto action: actions) {
        delete action;
      }
      return;
    }
    if(node) {
      QMenu contextMenu("Context menu");
      //fprintf(stderr, "node %d\n", node);

      contextNode = node;
      QAction action1("delete node", this);
      connect(&action1, SIGNAL(triggered()), this, SLOT(contextRemoveNode()));
      contextMenu.addAction(&action1);

      std::vector<std::string> cStrings = mainLib->getNodeContextStrings(node->getName());
      std::vector<QAction*> actions;
      for(auto label: cStrings) {
        QAction *action = new QAction(label.c_str(), this);
        connect(action, SIGNAL(triggered()), this, SLOT(nodeContextClicked()));
        contextMenu.addAction(action);
      }
      contextMenu.exec(viz->mapToGlobal(QPoint(x*viz->width(),
                                               (1-y)*viz->height())));
      for(auto action: actions) {
        delete action;
      }
      return;
    }
    if(edge) {
      QMenu contextMenu("Context menu");
      //fprintf(stderr, "node %d\n", node);
      contextEdge = edge;
      QAction action1("delete edge", this);
      connect(&action1, SIGNAL(triggered()), this, SLOT(contextRemoveEdge()));
      contextMenu.addAction(&action1);
      // todo convert to correct position
      contextMenu.exec(viz->mapToGlobal(QPoint(x*viz->width(),
                                               (1-y)*viz->height())));
      return;
    }
  }

  void View::contextRemoveNode() {
    if(contextNode.valid()) {
      view->removeNode(contextNode.get());
    }
  }

  void View::contextRemoveEdge() {
    if(contextEdge.valid()) {
      view->removeEdge(contextEdge.get());
    }
  }

  void View::toggleInputInterface() {
    if(contextNode.valid() && contextPort >= 0) {
      ConfigMap map = contextNode->getMap();
      int i = 0;
      if(map["inputs"][contextPort].hasKey("interface")) {
        i = map["inputs"][contextPort]["interface"];
      }
      ++i %= 3;
      map["inputs"][contextPort]["interface"] = i;
      if(!map["inputs"][contextPort].hasKey("interfaceExportName")) {
        map["inputs"][contextPort]["interfaceExportName"] = (std::string)map["name"] + ":" + (std::string)map["inputs"][contextPort]["name"];
      }
      if(!map["inputs"][contextPort].hasKey("initValue")) {
        map["inputs"][contextPort]["initValue"] = 0.0;
      }
      contextNode->updateMap(map);
      if(updateNodeId == nodeIdMap[contextNode.get()]) {
        dWidget->updateConfigMap("", contextNode->getMap());
      }
    }
  }

  void View::toggleOutputInterface() {
    if(contextNode.valid() && contextPort >= 0) {
      ConfigMap map = contextNode->getMap();
      int i = 0;
      if(map["outputs"][contextPort].hasKey("interface")) {
        i = map["outputs"][contextPort]["interface"];
      }
      ++i %= 3;
      map["outputs"][contextPort]["interface"] = i;
      if(!map["outputs"][contextPort].hasKey("interfaceExportName")) {
        map["outputs"][contextPort]["interfaceExportName"] = (std::string)map["name"] + ":" + (std::string)map["outputs"][contextPort]["name"];
      }
      contextNode->updateMap(map);
      if(updateNodeId == nodeIdMap[contextNode.get()]) {
        dWidget->updateConfigMap("", contextNode->getMap());
      }
    }
  }

  void View::removeNode(const std::string &name) {
    osg::ref_ptr<osg_graph_viz::Node> node = getNodeByName(name);
    if(node.valid()) {
      view->removeNode(node.get());
    }
  }

  void View::nodeContextClicked() {
    QAction *action = dynamic_cast<QAction*>(sender());
    if(action) {
      mainLib->nodeContextClicked(action->text().toStdString());
    }
  }

  void View::inPortContextClicked() {
    QAction *action = dynamic_cast<QAction*>(sender());
    if(action) {
      mainLib->inPortContextClicked(action->text().toStdString());
    }
  }

  void View::outPortContextClicked() {
    QAction *action = dynamic_cast<QAction*>(sender());
    if(action) {
      mainLib->outPortContextClicked(action->text().toStdString());
    }
  }

} // end of namespace bagel_gui
