/**
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \version 0.1
 * \brief The BagelGui class is the main class of the graph GUI.
 *
 */

#ifndef BAGEL_GUI_BAGEL_GUI_HPP
#define BAGEL_GUI_BAGEL_GUI_HPP

// set define if you want to extend the gui
#include <lib_manager/LibInterface.hpp>

#include <mars/cfg_manager/CFGManagerInterface.h>
#include "NodeLoader.hpp"
#include "ModelInterface.hpp"
#include "PluginInterface.hpp"
#include "View.hpp"
#include <string>
#include <mars/main_gui/MenuInterface.h>
#include <mars/main_gui/GuiInterface.h>
#include <mars/main_gui/BaseWidget.h>
#include <osg_graph_viz/View.hpp>
#include <osgViewer/CompositeViewer>
#ifdef BGM
#include <mars/plugins/BehaviorGraphMARS/BehaviorGraphMARS.h>
#endif
#include <QTabWidget>

namespace bagel_gui {

  class SlotWrapper;
  class NodeTypeWidget;
  class NodeInfoWidget;
  class HistoryWidget;
  class GraphicsTimer;

  // inherit from MarsPluginTemplateGUI for extending the gui
  class BagelGui:  public lib_manager::LibInterface,
                   public mars::main_gui::MenuInterface,
                   public mars::cfg_manager::CFGClient {

  public:
    BagelGui(lib_manager::LibManager *theManager);
    ~BagelGui();

    // LibInterface methods
    int getLibVersion() const
    { return 1; }
    const std::string getLibName() const
    { return std::string("bagel_gui"); }
    CREATE_MODULE_INFO();

    void init();

    // MenuInterface methods
    void menuAction(int action, bool checked = false);

    void cfgUpdateProperty(mars::cfg_manager::cfgPropertyStruct _property);

    // BagelGui methods
    void load(const std::string &filename);
    void load(configmaps::ConfigMap &map, bool reload = false);
    void save(const std::string &filename);
    void exportCndFile(const std::string &filename);
    void addNode(const std::string &type, std::string name = "", double x = 0.0, double y = 0.0);
    void update(const std::string &filename);
    void updateMap(const configmaps::ConfigMap &map);
    void updateNodeTypes();
    void loadHistory(size_t index);
    bool groupNodes(const std::string &parent, const std::string &child);

    void setDirectLineMode();
    void setOrthoLineMode();
    void setSmoothLineMode();
    void decouple();
    void repositionNodes();
    void repositionEdges();
    void decoupleEdgesOfSelectedNodes();
    void createInputPortsForSelection(bool usePortNames);
    void createOutputPortsForSelection();
    void connectLoopPortsOfSelectedNodes();
    void loadSubFile(const std::string &file);
    void updateViewer();
    std::string getLoadPath();

    void openDebug();

    void addNode(osg_graph_viz::NodeInfo *info, double x, double y,
                 unsigned long *id, bool onLoad = false, bool reload=false);
    void addEdge(configmaps::ConfigMap edgeMap, bool reload=false);
    bool hasEdge(configmaps::ConfigMap edgeMap);
    osg_graph_viz::NodeInfo getNodeInfo(const std::string &name);
    void addSubgraphInfo(const std::string &filename, const std::string &absPath);
    void addModelInterface(std::string modelName, ModelInterface* model);
    void createView(const std::string &modelName, const std::string &tabName);
    std::string getConfigDir();
    void setModel(std::string model);
    void setTab(int index);
    void closeTab(int index);
    void closeCurrentTab();
    ModelInterface* getCurrentModel();

    std::string getNodeName(unsigned long id);
    std::string getInPortName(std::string nodeName, unsigned long index);
    std::string getOutPortName(std::string nodeName, unsigned long index);
    void updateNodeMap(const std::string &nodeName,
                       const configmaps::ConfigMap &map);
    const configmaps::ConfigMap* getNodeMap(const std::string &nodeName);
    void mapChanged();
    void updateViewSize();
    void setViewFilter(const std::string &filter, int value);
    void loadLayout(const std::string &file);
    void saveLayout(const std::string &file);
    configmaps::ConfigMap getLayout();
    void applyLayout(configmaps::ConfigMap &layout);
    void setExternNodePath(const std::string &rootPath, const std::string &path);
    void nodeTypeSelected(const std::string &nodeType);
    configmaps::ConfigMap createConfigMap();
    void setLoadPath(const std::string &path);
    void addPlugin(PluginInterface *plugin);
    void removePlugin(PluginInterface *plugin);
    void removeNode(const std::string &name);
    void nodeContextClicked(const std::string s);
    void inPortContextClicked(const std::string s);
    void outPortContextClicked(const std::string s);
    std::vector<std::string> getNodeContextStrings(std::string name);
    std::vector<std::string> getInPortContextStrings(std::string nodeName, std::string portName);
    std::vector<std::string> getOutPortContextStrings(std::string nodeName, std::string portName);

    configmaps::ConfigMap getGlobalConfig();
    void setGlobalConfig(configmaps::ConfigMap &config);
    void updateGlobalConfig(configmaps::ConfigMap &config);

  private:
    mars::main_gui::BaseWidget *dwBase;
    mars::config_map_gui::DataWidget *dw;
    mars::main_gui::GuiInterface *gui;
#ifdef BGM
    mars::plugins::BehaviorGraphMARS::BehaviorGraphMARS *bgMars;
#endif
    std::string loadPath, loadedGraphFile;
    configmaps::ConfigMap config, globalConfig;
    GraphicsTimer *timer;
    QTabWidget *mainWidget;
    SlotWrapper *slotWrapper;
    NodeTypeWidget *ntWidget;
    NodeInfoWidget *niWidget;
    HistoryWidget *hWidget;
    mars::cfg_manager::cfgPropertyStruct example, width, height;
    osg::ref_ptr<osg_graph_viz::View> view;
    double retinaScale;
    bool updateSize;

    bool autoUpdate;
    std::string confDir, resourcesPath, resourcesPathConfig;
    osgViewer::CompositeViewer *viewer;

    NodeLoader *loader;

    std::vector<PluginInterface*> plugins;
    std::map<std::string, ModelInterface*> modelMap;
    std::map<std::string, View*> tabMap;

    osg::observer_ptr<osg::GraphicsContext> sharedGLContext;
    View *currentTabView;
    std::vector<std::string> menuModelList;
    bool useForceLayout;
    double devicePixelRatio_;

    void toggleWidget(mars::main_gui::BaseWidget *w);
    void menuLoad();
    void menuSave();
    void menuAddSubgraph();
    void menuCreateTab(int index);
    void menuLoadLayout();
    void menuSaveLayout();
    void menuImportSmurf();
    void menuDecoupleLong();
    void menuExportCnd();
    void menuExportSvg();

  }; // end of class definition BagelGui

} // end of namespace bagel_bui

#endif // BAGEL_GUI_OSGBG_HPP
