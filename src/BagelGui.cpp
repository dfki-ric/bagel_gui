/**
 * \file BagelGui.cpp
 * \author Malte Langosz (malte.langosz@dfki.de)
 *
 * Version 0.1
 *
 */


#include "GraphicsTimer.hpp"
#include "BagelGui.hpp"
#include "BagelLoader.hpp"
#include "BagelModel.hpp"
#include "SlotWrapper.hpp"
#include "NodeTypeWidget.hpp"
#include "NodeInfoWidget.hpp"
#include "HistoryWidget.hpp"

#include <mars/utils/misc.h>

#include <QHBoxLayout>
#include <QSplitter>
#include <QFileDialog>

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

  BagelGui::BagelGui(lib_manager::LibManager *theManager)
    : lib_manager::LibInterface(theManager) {
    gui = libManager->getLibraryAs<mars::main_gui::GuiInterface>("main_gui");
    currentTabView = NULL;
    init();
  }

  std::string BagelGui::getConfigDir() {
    return confDir;
  }

  void BagelGui::init() {
    updateSize = false;
    autoUpdate = false;
    mars::cfg_manager::CFGManagerInterface *cfg;
    cfg = libManager->getLibraryAs<mars::cfg_manager::CFGManagerInterface>("cfg_manager");
    // the mouse movement between osx and linux is inverted on the y axis
    // this is corrected by mars_graphics but result in an offset
    // after scaling the value for osg_graph_viz
    confDir = ".";
    resourcesPath = ".";
    if(cfg) {
      resourcesPath = cfg->getOrCreateProperty("Preferences",
                                               "resources_path",
                                               ".").sValue;
      cfg->getPropertyValue("Config", "config_path", "value", &confDir);
    }
    fprintf(stderr, "configPath: %s\n", confDir.c_str());
    config = ConfigMap::fromYamlFile(confDir+"/config_default.yml", true);
    if(mars::utils::pathExists(confDir+"/config.yml")) {
      config.append(ConfigMap::fromYamlFile(confDir+"/config.yml", true));
    }
    if(!config.hasKey("retinaScale")) {
      config["retinaScale"] = 1.0;
    }
    if(!config.hasKey("PortIconScale")) {
      config["PortIconScale"] = 1.0;
    }
    if(!config.hasKey("ClassicLook")) {
      config["ClassicLook"] = false;
    }
    cfg->getOrCreateProperty("bagel_gui", "retinaScale",
                             (double)config["retinaScale"], this);
    std::string icon = resourcesPath + "/bagel_gui/resources/images/";
    gui->setWindowTitle("Graph Editor");
    gui->addGenericMenuAction("../File/Load", 1, this);
    gui->addGenericMenuAction("../File/Save", 2, this);
    gui->addGenericMenuAction("../File/Export Svg", 24, this);
    gui->addGenericMenuAction("../File/Bagel/AddSubgraphType", 6, this);
    //gui->addGenericMenuAction("../File/Bagel/Import Smurf", 16, this);
    //gui->addGenericMenuAction("../File/export/cnd_model", 20, this);
    gui->addGenericMenuAction("../Windows/DataView", 3, this, 0,
                              icon+"DataView.png", true);
    gui->addGenericMenuAction("../Windows/NodeTypes", 4, this, 0,
                              icon+"NodeTypes.png", true);
    gui->addGenericMenuAction("../Windows/NodeInfo", 23, this);
    gui->addGenericMenuAction("../Windows/History", 5, this, 0,
                              icon+"History.png", true);
    gui->addGenericMenuAction("../Edit/DirectLineMode", 7, this, 0, "", 0, 1);
    gui->setMenuActionSelected("../Edit/DirectLineMode", true);
    gui->addGenericMenuAction("../Edit/SmoothLineMode", 21, this, 0, "", 0, 1);
    gui->setMenuActionSelected("../Edit/SmoothLineMode", true);
    gui->addGenericMenuAction("../Edit/Decouple Selected", 8, this);
    gui->addGenericMenuAction("../Edit/Decouple Long Edges", 19, this);
    gui->addGenericMenuAction("../Edit/Decouple Edges Of Selected Nodes", 26, this);
    gui->addGenericMenuAction("../Edit/Reposition Nodes", 9, this);
    gui->addGenericMenuAction("../Edit/Reposition Edges", 10, this);
    gui->addGenericMenuAction("../Edit/Set Edges Smooth", 22, this);
    gui->addGenericMenuAction("../Edit/Create Input for Selected Nodes", 11, this);
    gui->addGenericMenuAction("../Edit/Create Input for Selected Ports", 17, this);
    gui->addGenericMenuAction("../Edit/Create Output for Selected Ports", 25, this);
    gui->addGenericMenuAction("../Edit/Connect loop ports for Selected Nodes", 27, this);
    gui->addGenericMenuAction("../Edit/Use Force Positioning", 12, this,
                              0, "", 0, 1);
    gui->addGenericMenuAction("../Views/Reset View", 18, this);
    gui->addGenericMenuAction("../Views/Load Layout", 14, this);
    gui->addGenericMenuAction("../Views/Save Layout", 15, this);
    gui->addGenericMenuAction("../Views/Open Debug View", 13, this);


    dwBase = new mars::main_gui::BaseWidget(NULL, cfg, "NodeData");
    dw = new mars::config_map_gui::DataWidget(NULL, NULL, true);
    QVBoxLayout *l = new QVBoxLayout(dwBase);
    l->addWidget(dw);
    l->setContentsMargins(0, 0, 0, 0);
    // for demo purpose
    /*
    dw = new mars::config_map_gui::DataWidget(cfg, 0, true, false);
    std::vector<std::string> editPattern;
    editPattern.push_back("../name"); // fake patter to activate check
    dw->setEditPattern(editPattern);
    */
    ntWidget = new NodeTypeWidget(cfg, this);
    ntWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    niWidget = new NodeInfoWidget(cfg, this);
    niWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    hWidget = new HistoryWidget(cfg, this);
    hWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    addModelInterface("bagel", new BagelModel(this));

    { // setup composite viewer
      viewer = new osgViewer::CompositeViewer();
      viewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
      viewer->setKeyEventSetsDone(0);
    }

    mainWidget = NULL;
    mainWidget = new QTabWidget();
    mainWidget->setTabsClosable(true);
    mars::main_gui::BaseWidget *widget2 = new mars::main_gui::BaseWidget(0, cfg, "graph_control");
    QHBoxLayout *layout = new QHBoxLayout();
    mainWidget->setStyleSheet("padding:0px;");

    layout = new QHBoxLayout();
    layout->setContentsMargins(1, 1, 1, 1);
    layout->addWidget(mainWidget);

    widget2->setStyleSheet("padding:0px;");
    widget2->setLayout(layout);

    gui->addDockWidget((void*)widget2, 1, 0, true);

    if(!ntWidget->getHiddenCloseState())
      gui->addDockWidget((void*)ntWidget, 1);
    if(!niWidget->getHiddenCloseState())
      gui->addDockWidget((void*)niWidget, 1);
    if(!hWidget->getHiddenCloseState())
      gui->addDockWidget((void*)hWidget, 1);
    if(!dwBase->getHiddenCloseState())
      gui->addDockWidget((void*)dwBase, 1, 4);

    slotWrapper = new SlotWrapper(this);
    QObject::connect(mainWidget, SIGNAL(currentChanged(int)), slotWrapper, SLOT(tabChanged(int)));
    QObject::connect(mainWidget, SIGNAL(tabCloseRequested(int)), slotWrapper, SLOT(closeTab(int)));

    QObject::connect(dw, SIGNAL(mapChanged()), slotWrapper, SLOT(mapChanged()));
    char label[55];
    int i=0;
    // parse arguments
    std::string arg;
    std::string loadGraph;
    sprintf(label, "arg%d", i++);
    while (cfg->getPropertyValue("Config", label, "value", &arg)) {
      if(arg.find("dock") != std::string::npos) {
        gui->dock(true);
      }
      sprintf(label, "arg%d", i++);
    };
    cfg->getPropertyValue("Config", "graph", "value", &loadGraph);

    //gui->dock(true);
    gui->show();

    loader = new BagelLoader(this);

    timer = new GraphicsTimer(this);

#ifdef BGM
    bgMars = libManager->getLibraryAs<mars::plugins::BehaviorGraphMARS::BehaviorGraphMARS>("BehaviorGraphMARS");
#endif
    if(!loadGraph.empty()) {
      loadPath = mars::utils::getPathOfFile(loadGraph);
      if(loadPath[loadPath.size()-1] != '/') loadPath.append("/");

      fprintf(stderr, "load graph: %s\n", loadGraph.c_str());
      load(loadGraph);
    }
    else {
      createView("bagel", "Bagel Graph");
    }
  }

  BagelGui::~BagelGui() {
    libManager->releaseLibrary("main_gui");
    libManager->releaseLibrary("cfg_manager");

#ifdef BGM
    if(bgMars) {
      libManager->releaseLibrary("BehaviorGraphMARS");
    }
#endif
    delete viewer;
    delete timer;
    delete slotWrapper;
#ifndef USE_QT5
    if(mainWidget) delete mainWidget;
#endif
    //if(slotWrapper) delete slotWrapper;
  }

  void BagelGui::update(const std::string &filename) {
#ifdef BGM
    if(bgMars) {
      bgMars->update(filename, createConfigMap());
      autoUpdate = true;
    }
#endif
  }

  void BagelGui::toggleWidget(mars::main_gui::BaseWidget *w) {
    if(w->isHidden()) {
      gui->addDockWidget((void*)w, 1);
    }
    else {
      gui->removeDockWidget((void*)w, 1);
    }
  }

  void BagelGui::menuLoad() {
    QString fileName = QFileDialog::getOpenFileName(NULL,
                                                    QObject::tr("Select Graph"),
                                                    loadPath.c_str(),
                                                    QObject::tr("Graph Files (*.yml)"),0,QFileDialog::DontUseNativeDialog);
    if(!fileName.isNull()) {
      load(fileName.toStdString());
    }
  }

  void BagelGui::menuSave() {
    QString fileName = QFileDialog::getSaveFileName(NULL,
                                                    QObject::tr("Select Graph"),
                                                    loadPath.c_str(),
                                                    QObject::tr("Graph Files (*.yml)"),0,QFileDialog::DontUseNativeDialog);
    if(!fileName.isNull()) {
      loadPath = mars::utils::getPathOfFile(fileName.toStdString());
      save(fileName.toStdString());
    }
  }

  void BagelGui::menuExportCnd() {
    QString fileName = QFileDialog::getSaveFileName(NULL,
                                                    QObject::tr("Select Graph"),
                                                    loadPath.c_str(),
                                                    QObject::tr("Graph Files (*.yml)"),0,QFileDialog::DontUseNativeDialog);
    if(!fileName.isNull()) {
      loadPath = mars::utils::getPathOfFile(fileName.toStdString());
      loader->exportCnd(createConfigMap(), fileName.toStdString());
    }
  }

  void BagelGui::menuExportSvg() {
    QString fileName = QFileDialog::getSaveFileName(NULL,
                                                    QObject::tr("Select File"),
                                                    loadPath.c_str(),
                                                    QObject::tr("Graph Files (*.svg)"),0,QFileDialog::DontUseNativeDialog);
    if(!fileName.isNull()) {
      std::string f = fileName.toStdString();
      currentTabView->getView()->exportSvg(f);
    }
  }

  void BagelGui::menuAddSubgraph() {
    QString fileName = QFileDialog::getOpenFileName(NULL,
                                                    QObject::tr("Select Graph"),
                                                    loadPath.c_str(),
                                                    QObject::tr("Graph Files (*.yml)"),0,QFileDialog::DontUseNativeDialog);
    if(!fileName.isNull()) {
      loadSubFile(fileName.toStdString());
    }
  }

  void BagelGui::menuImportSmurf() {
    if(currentTabView) {
      ModelInterface *model = currentTabView->getModel();
      BagelModel *bagelModel = dynamic_cast<BagelModel*>(model);
      if(bagelModel) {
        QString fileName = QFileDialog::getOpenFileName(NULL,
                                                        QObject::tr("Select Smurf"),
                                                        loadPath.c_str(),
                                                        QObject::tr("Smurf Files (*.smurf)"),0);
        if(!fileName.isNull()) {
          currentTabView->saveLayout();
          bagelModel->importSmurf(fileName.toStdString());
          currentTabView->restoreLayout();
        }
      }
    }
  }

  void BagelGui::menuLoadLayout() {
    if(currentTabView) {
      QString fileName = QFileDialog::getOpenFileName(NULL,
                                                      QObject::tr("Select Layout"),
                                                      loadPath.c_str(),
                                                      QObject::tr("Layout Files (*.yml)"),0,QFileDialog::DontUseNativeDialog);
      loadLayout(fileName.toStdString());
    }
  }

  void BagelGui::loadLayout(const std::string &file) {
    if(currentTabView) {
      std::string loadFile = file;
      if(file[0] != '/') {
        loadFile = loadPath + file;
      }
      currentTabView->loadLayout(loadFile);
    }
  }

  void BagelGui::menuSaveLayout() {
    if(currentTabView) {
      QString fileName = QFileDialog::getSaveFileName(NULL,
                                                      QObject::tr("Select Layout"),
                                                      loadPath.c_str(),
                                                      QObject::tr("Layout Files (*.yml)"),0,QFileDialog::DontUseNativeDialog);
      if(!fileName.isNull()) {
        saveLayout(fileName.toStdString());
      }
    }
  }

  void BagelGui::saveLayout(const std::string &file) {
    if(currentTabView) {
      std::string loadFile = file;
      if(file[0] != '/') {
        loadFile = loadPath + file;
      }
      currentTabView->saveLayout(loadFile);
    }
  }

  ConfigMap BagelGui::getLayout() {
    if(currentTabView) {
      return currentTabView->getLayout();
    }
    return ConfigMap();
  }

  void BagelGui::applyLayout(ConfigMap &layout) {
    if(currentTabView) {
      currentTabView->applyLayout(layout);
    }
  }

  void BagelGui::menuCreateTab(int index) {
    static unsigned int viewNum = 0;
    std::stringstream ss;
    ss << menuModelList[index] << "_" << ++viewNum;
    createView(menuModelList[index], ss.str());
  }

  void BagelGui::menuDecoupleLong() {
    if(currentTabView) {
      currentTabView->getView()->decoupleLongEdges();
    }
  }

  void BagelGui::menuAction(int action, bool checked) {
    if(action >= 50) {
      // reserved for views
      menuCreateTab(action-50);
    }
    switch(action) {
    case 1: { // load
      menuLoad();
      break;
    }
    case 2: { // save
      menuSave();
      break;
    }
    case 3: {
      toggleWidget(dwBase);
      break;
    }
    case 4: {
      toggleWidget(ntWidget);
      break;
    }
    case 23: {
      toggleWidget(niWidget);
      break;
    }
    case 5: {
      toggleWidget(hWidget);
      break;
    }
    case 6: { // save
      menuAddSubgraph();
      break;
    }
    case 7: {
      if(checked) setDirectLineMode();
      else setOrthoLineMode();
      break;
    }
    case 21: {
      if(checked) setSmoothLineMode();
      else setDirectLineMode();
      break;
    }
    case 8: {
      decouple();
      break;
    }
    case 9: {
      repositionNodes();
      break;
    }
    case 10: {
      repositionEdges();
      break;
    }
    case 11: {
      createInputPortsForSelection(false);
      break;
    }
    case 12: {
      if(currentTabView) {
        currentTabView->setUseForceLayout(checked);
      }
      break;
    }
    case 13: {
      openDebug();
      break;
    }
    case 14: {
      menuLoadLayout();
      break;
    }
    case 15: {
      menuSaveLayout();
      break;
    }
    case 16: {
      menuImportSmurf();
      break;
    }
    case 17: {
      createInputPortsForSelection(true);
      break;
    }
    case 18: {
      if(currentTabView && currentTabView->getView()) {
        currentTabView->getView()->setViewPos();
      }
      break;
    }
    case 19: {
      menuDecoupleLong();
      break;
    }
    case 20: {
      menuExportCnd();
      break;
    }
    case 22: {
      if(currentTabView) {
        currentTabView->getView()->setEdgesSmooth(true);
      }
      break;
    }
    case 24: {
      if(currentTabView) {
        //currentTabView->getView()->exportSvg("/Users/maltel/Desktop/foo2.svg");
        menuExportSvg();
      }
      break;
    }
    case 25: {
      createOutputPortsForSelection();
      break;
    }
    case 26: {
      decoupleEdgesOfSelectedNodes();
      break;
    }
    case 27: {
      connectLoopPortsOfSelectedNodes();
      break;
    }
    }
  }

  void BagelGui::loadHistory(size_t index) {
    if(currentTabView) currentTabView->loadHistory(index);
  }

  void BagelGui::updateMap(const ConfigMap &map) {
    if(currentTabView) {
      currentTabView->updateMap(map);
      if(autoUpdate) {
        update(loadedGraphFile);
      }
    }
  }

  void BagelGui::updateNodeMap(const std::string &nodeName, const ConfigMap &map) {
    if(currentTabView) {
      currentTabView->updateNodeMap(nodeName, map);
    }
  }

  const configmaps::ConfigMap* BagelGui::getNodeMap(const std::string &nodeName) {
    if(currentTabView) {
      return currentTabView->getNodeMap(nodeName);
    }
    return NULL;
  }

  void BagelGui::load(const std::string &filename) {
    autoUpdate = false;
    loadedGraphFile = filename;
    loadPath = mars::utils::getPathOfFile(filename);
    if(loadPath[loadPath.size()-1] != '/') loadPath.append("/");
    fprintf(stderr, "set load path to: %s\n", loadPath.c_str());

    createView("", trim(filename));
    loader->load(filename);
    currentTabView->addHistoryEntry(filename);
  }

  void BagelGui::addNode(const std::string &type, std::string name,
                      double x, double y) {
    if(currentTabView) currentTabView->addNode(type, name, x, y);
  }

  // This method is called from loading or import functionality
  void BagelGui::addNode(osg_graph_viz::NodeInfo *info, double x, double y,
                         unsigned long *id, bool onLoad, bool reload) {
    if(currentTabView) currentTabView->addNode(info, x, y, id, onLoad, reload);
  }

  std::string BagelGui::getNodeName(unsigned long id) {
    if(currentTabView) return currentTabView->getNodeName(id);
    return std::string();
  }

  std::string BagelGui::getInPortName(std::string nodeName, unsigned long index) {
    if(currentTabView) return currentTabView->getInPortName(nodeName, index);
    return std::string();
  }

  std::string BagelGui::getOutPortName(std::string nodeName, unsigned long index) {
    if(currentTabView) return currentTabView->getOutPortName(nodeName, index);
    return std::string();
  }

  void BagelGui::addEdge(ConfigMap edgeMap, bool reload) {
    if(currentTabView) currentTabView->addEdge(edgeMap, reload);
  }

  bool BagelGui::hasEdge(ConfigMap edgeMap) {
    if(currentTabView) return currentTabView->hasEdge(edgeMap);
  }

  osg_graph_viz::NodeInfo BagelGui::getNodeInfo(const std::string &name) {
    if(currentTabView) return currentTabView->getNodeInfo(name);
    return osg_graph_viz::NodeInfo();
  }

  void BagelGui::load(ConfigMap &map, bool reload) {
    if(currentTabView) {
      currentTabView->clearGraph();
      autoUpdate = false;
      loader->load(map, loadPath, reload);
      if(!reload) {
        fprintf(stderr, "load completed\n");
      }
    }
  }

  std::string BagelGui::getLoadPath(){
    return loadPath;
  }

  void BagelGui::loadSubFile(const std::string &file) {
    if(currentTabView) {
      std::string filename = file;

      // get the subrgraph name and its absolute path
      std::string absPath = mars::utils::getPathOfFile(filename);
      if(absPath[absPath.size()-1] != '/') absPath.append("/");
      mars::utils::removeFilenamePrefix(&filename);
      addSubgraphInfo(filename, absPath);
    }
  }

  void BagelGui::addModelInterface(std::string modelName, ModelInterface* model) {
    modelMap[modelName] = model;
    std::string entry = "New " + modelName + " Tab";
    gui->addGenericMenuAction("../Views/"+entry, 50+menuModelList.size(), this);
    menuModelList.push_back(modelName);
  }

  void BagelGui::setModel(const std::string model) {
    if(currentTabView) {
      if(modelMap.find(model) != modelMap.end()) {
        currentTabView->setModel(modelMap[model]->clone(), model);
      }
    }
  }

  void BagelGui::createView(const std::string &modelName,
                         const std::string &tabName) {
    static bool first = true;
    autoUpdate = false;


    View *v = new View(this, sharedGLContext, ntWidget, hWidget, dw,
                       confDir, resourcesPath+"/osg_graph_viz/",
                       (double) config["HeaderFontSize"],
                       (double) config["PortFontSize"],
                       (double) config["PortIconScale"],
                       (bool) config["ClassicLook"]);

    osgViewer::View *osgView = v->getOsgView();
    osg_graph_viz::View *view = v->getView();
    view->setLineMode(osg_graph_viz::SMOOTH_LINE_MODE);
    viewer->addView(osgView);
    QWidget *viz = v->getWidget();
    viz->setMinimumWidth(200);
    viz->setMinimumHeight(200);
    currentTabView = v;
    int i=0;
    std::stringstream ss;
    ss << ++i << ":" << tabName;
    std::string s = ss.str();
    while(tabMap.find(s) != tabMap.end()) {
      ss.str("");
      ss << ++i << ":" << tabName;
      s = ss.str();
    }
    tabMap[s] = v;
    if(!modelName.empty()) {
      setModel(modelName);
    }
    int index = mainWidget->addTab(viz, s.c_str());
    mainWidget->setCurrentIndex(index);
    view->setRetinaScale(config["retinaScale"]);
    fprintf(stderr, "dada: %d %d\n", viz->width(), viz->height());
    viz->resize(viz->width(), viz->height());
    devicePixelRatio_ = 1.0;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 2, 0))
      devicePixelRatio_ = viz->devicePixelRatio();
#endif
    view->resize(viz->width()*devicePixelRatio_,
                 viz->height()*devicePixelRatio_);
    // for newer Qt versions we need to draw a first frame to be able
    // to init new tabs
    if(first) {
      first = false;
      viewer->frame();
    }
    if(!timer->isRunning()) {
      timer->run();
    }
    if(first) {
      first = false;
    }
  }

  void BagelGui::addSubgraphInfo(const std::string &filename, const std::string &absPath) {
    if(currentTabView) {
      if(currentTabView->getModel()->loadSubgraphInfo(filename, absPath)) {
        currentTabView->updateWidgets();
      }
    }
  }

  ConfigMap BagelGui::createConfigMap() {
    return currentTabView->createConfigMap();
  }

  void BagelGui::save(const std::string &filename) {
    std::string fileName = filename;
    if(mars::utils::getFilenameSuffix(fileName) == "") {
      fileName += ".yml";
    }
    loader->save(createConfigMap(), fileName);
  }

  void BagelGui::exportCndFile(const std::string &filename) {
    std::string fileName = filename;
    if(mars::utils::getFilenameSuffix(fileName) == "") {
      fileName += ".yml";
    }
    loader->exportCnd(createConfigMap(), fileName);
  }

  void BagelGui::setDirectLineMode() {
    if(currentTabView)
      currentTabView->getView()->setLineMode(osg_graph_viz::DIRECT_LINE_MODE);
  }

  void BagelGui::setOrthoLineMode() {
    if(currentTabView)
      currentTabView->getView()->setLineMode(osg_graph_viz::ORTHO_LINE_MODE);
  }

  void BagelGui::setSmoothLineMode() {
    if(currentTabView)
      currentTabView->getView()->setLineMode(osg_graph_viz::SMOOTH_LINE_MODE);
  }

  void BagelGui::decouple() {
    if(currentTabView)
      currentTabView->getView()->decoupleSelected();
  }

  void BagelGui::repositionNodes() {
    if(currentTabView)
      currentTabView->getView()->repositionNodes();
  }

  void BagelGui::repositionEdges() {
    if(currentTabView)
      currentTabView->getView()->repositionEdges();
  }

  void BagelGui::decoupleEdgesOfSelectedNodes() {
    if(currentTabView) {
        osg::ref_ptr<osg_graph_viz::View> view = currentTabView->getView();
        view->decoupleEdgesOfNodes(view->getSelectedNodes());
    }
  }

  // bagel specific function
  void BagelGui::createInputPortsForSelection(bool usePortNames) {
    if(currentTabView) {
      ModelInterface *model = currentTabView->getModel();
      BagelModel *bagelModel = dynamic_cast<BagelModel*>(model);
      if(bagelModel) {
        osg::ref_ptr<osg_graph_viz::View> view = currentTabView->getView();
        bagelModel->createInputPortsForSelection(view->getSelectedNodes(),
                                                 config["PortFontSize"],
                                                 config["HeaderFontSize"],
                                                 usePortNames);
      }
    }
  }

  // bagel specific function
  void BagelGui::createOutputPortsForSelection() {
    if(currentTabView) {
      ModelInterface *model = currentTabView->getModel();
      BagelModel *bagelModel = dynamic_cast<BagelModel*>(model);
      if(bagelModel) {
        osg::ref_ptr<osg_graph_viz::View> view = currentTabView->getView();
        bagelModel->createOutputPortsForSelection(view->getSelectedNodes(),
                                                  config["PortFontSize"],
                                                  config["HeaderFontSize"]);
      }
    }
  }

  void BagelGui::connectLoopPortsOfSelectedNodes() {
    if(currentTabView) {
      osg::ref_ptr<osg_graph_viz::View> view = currentTabView->getView();
      std::list<osg::ref_ptr<osg_graph_viz::Node> > selectedNodes = view->getSelectedNodes();
      for(auto it=selectedNodes.begin(); it!=selectedNodes.end(); ++it) {
        osg_graph_viz::NodeInfo ni = (*it)->getNodeInfo();
        for (int out = 0; out<ni.numOutputs; out++) {
          for (int in = 0; in<ni.numInputs; in++) {
            if ((*it)->getOutPortName(out) == (*it)->getInPortName(in)) {
              configmaps::ConfigMap edgeMap;
              edgeMap["toNode"] = edgeMap["fromNode"] = (*it)->getName();
              edgeMap["toNodeInput"] = edgeMap["fromNodeOutput"] = (*it)->getOutPortName(out);
              edgeMap["smooth"] = true;
              edgeMap["ignore_for_sort"] = 1;
              if (!hasEdge(edgeMap)) addEdge(edgeMap);
            }
          }
        }
      }
    }
  }

  void BagelGui::updateViewer() {
    if(updateSize) {
      updateSize = false;
      if(currentTabView) {
        osg_graph_viz::View *view = currentTabView->getView();
        QWidget *viz = currentTabView->getWidget();
        viz->resize(viz->width(), viz->height());
        view->resize(devicePixelRatio_*viz->width(),
                     devicePixelRatio_*viz->height());
        fprintf(stderr, "resize: %d %d\n", viz->width(), viz->height());
      }
    }

    // todo: update frame only if needed?
    viewer->frame();
    if(currentTabView) {
      osg_graph_viz::View *view = currentTabView->getView();
      if(view) view->update();
      currentTabView->forceDirectedLayoutStep();
    }

  }

  void BagelGui::updateNodeTypes()
  {
    if(currentTabView) {
        currentTabView->updateWidgets();
    }
  }

  void BagelGui::setTab(int index) {
    if(index < 0) {
      currentTabView = 0;
      return;
    }
    currentTabView = tabMap[mainWidget->tabText(index).toStdString()];
    currentTabView->updateWidgets();
    QWidget *viz = currentTabView->getWidget();
    currentTabView->getView()->resize(viz->width()*devicePixelRatio_,
                                      viz->height()*devicePixelRatio_);
    gui->setMenuActionSelected("../Edit/Use Force Positioning",
                               currentTabView->getUseForceLayout());

    ModelInterface *model = currentTabView->getModel();
    for(auto p: plugins) {
      p->currentModelChanged(model);
    }
  }

  void BagelGui::closeTab(int index) {
    std::string tabText = mainWidget->tabText(index).toStdString();
    View *tab = tabMap[tabText];
    osgViewer::View *osgView = tab->getOsgView();
    viewer->removeView(osgView);
    delete tab;
    mainWidget->removeTab(index);
    tabMap.erase(tabText);
    if(tabMap.empty()) {
      timer->stop();
    }
  }

  bool BagelGui::groupNodes(const std::string &parent, const std::string &child) {
    if(currentTabView) {
      return currentTabView->groupNodes(parent, child);
    }
    return false;
  }

  ModelInterface* BagelGui::getCurrentModel() {
    if(currentTabView) {
      return currentTabView->getModel();
    }
    return NULL;
  }

  void BagelGui::openDebug() {
    if(currentTabView) {
      currentTabView->getModel()->displayWidget( NULL );
    }
  }

  void BagelGui::mapChanged() {
    updateMap(dw->getConfigMap());
  }

  void BagelGui::cfgUpdateProperty(mars::cfg_manager::cfgPropertyStruct _property) {
    config["retinaScale"] = _property.dValue;
    std::map<std::string, View*>::iterator it =  tabMap.begin();
    for(; it!=tabMap.end(); ++it) {
      it->second->getView()->setRetinaScale(config["retinaScale"]);
    }
  }

  void BagelGui::updateViewSize() {
    updateSize = true;
  }

  void BagelGui::setViewFilter(const std::string &filter, int value) {
    if(currentTabView) {
      currentTabView->getView()->setFilter(filter, value);
    }
  }

  void BagelGui::setExternNodePath(const std::string &rootPath,
                                const std::string &path) {
    if(currentTabView) {
      BagelModel *model = dynamic_cast<BagelModel*>(currentTabView->getModel());
      if(model) {
        model->getExternNodes(path, rootPath);
        currentTabView->updateWidgets();
      }
    }
  }

  void BagelGui::nodeTypeSelected(const std::string &nodeType) {
    if(currentTabView) {
      ConfigMap map = currentTabView->getTypeInfo(nodeType);
      niWidget->setInfo(map);
    }
  }

  void BagelGui::setLoadPath(const std::string &path) {
    loadPath = path;
    if(loadPath[loadPath.size()-1] != '/') loadPath.append("/");
  }

  void BagelGui::addPlugin(PluginInterface *plugin) {
    auto result = find(plugins.begin(), plugins.end(), plugin);
    if(result == plugins.end()) plugins.push_back(plugin);
  }

  void BagelGui::removePlugin(PluginInterface *plugin) {
    std::vector<PluginInterface*>::iterator it = plugins.begin();
    for(; it!=plugins.end(); ++it) {
      if(*it == plugin) {
        plugins.erase(it);
        break;
      }
    }
  }

  void BagelGui::removeNode(const std::string &name) {
    if(currentTabView) {
      currentTabView->removeNode(name);
    }
  }

  void BagelGui::nodeContextClicked(const std::string s) {
    for(auto plugin: plugins) {
      plugin->nodeContextClicked(s);
    }
  }

  void BagelGui::inPortContextClicked(const std::string s) {
    for(auto plugin: plugins) {
      plugin->inPortContextClicked(s);
    }
  }

  void BagelGui::outPortContextClicked(const std::string s) {
    for(auto plugin: plugins) {
      plugin->outPortContextClicked(s);
    }
  }

  std::vector<std::string> BagelGui::getNodeContextStrings(std::string name) {
    std::vector<std::string> strings;
    for(auto plugin: plugins) {
      std::vector<std::string> r = plugin->getNodeContextStrings(name);
      for(auto s: r) {
        strings.push_back(s);
      }
    }
    return strings;
  }

  std::vector<std::string> BagelGui::getInPortContextStrings(std::string nodeName, std::string portName) {
    std::vector<std::string> strings;
    for(auto plugin: plugins) {
      std::vector<std::string> r = plugin->getInPortContextStrings(nodeName, portName);
      for(auto s: r) {
        strings.push_back(s);
      }
    }
    return strings;
  }

  std::vector<std::string> BagelGui::getOutPortContextStrings(std::string nodeName, std::string portName) {
    std::vector<std::string> strings;
    for(auto plugin: plugins) {
      std::vector<std::string> r = plugin->getOutPortContextStrings(nodeName, portName);
      for(auto s: r) {
        strings.push_back(s);
      }
    }
    return strings;
  }

} // end of namespace bagel_gui

DESTROY_LIB(bagel_gui::BagelGui)
CREATE_LIB(bagel_gui::BagelGui)
