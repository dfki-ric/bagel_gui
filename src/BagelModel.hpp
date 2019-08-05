/**
 * \file BagelLoader.hpp
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief Model implementation for Bagel
 *
 * Version 0.1
 */

#include "ModelInterface.hpp"

#ifndef BAGEL_GUI_BAGEL_MODEL_HPP
#define BAGEL_GUI_BAGEL_MODEL_HPP

namespace bagel_gui {

  class BagelGui;

  class BagelModel : public ModelInterface {
  public:
    BagelModel(BagelGui *bagelGui);
    virtual ~BagelModel() {}

    ModelInterface* clone();
    // load file or directory
    void loadNodeInfo(const std::string &filename);

    void getExternNodes(std::string path, std::string rootPath = "");
    void handlePotentialLibraryChanges(configmaps::ConfigVector::iterator node,
                                       std::string nodeName,
                                       osg_graph_viz::NodeInfo *info);
    void addExternNode(configmaps::ConfigMap externMap);

    // model interface methods
    bool addNode(unsigned long nodeId, configmaps::ConfigMap *node);
    bool addEdge(unsigned long egdeId, configmaps::ConfigMap *node);
    bool addNode(unsigned long nodeId, const configmaps::ConfigMap &node);
    bool addEdge(unsigned long egdeId, const configmaps::ConfigMap &edge);
    bool hasEdge(configmaps::ConfigMap *edge);
    bool hasEdge(const configmaps::ConfigMap &edge);
    void preAddNode(unsigned long nodeId);
    bool updateNode(unsigned long nodeId, configmaps::ConfigMap node);
    bool updateEdge(unsigned long egdeId, configmaps::ConfigMap edge);
    bool removeNode(unsigned long nodeId);
    bool removeEdge(unsigned long edgeId);
    bool handlePortCompatibility() {return false;}
    std::map<unsigned long, std::vector<std::string> > getCompatiblePorts(unsigned long nodeId, std::string outPortName) {return std::map<unsigned long, std::vector<std::string> >();}
    bool loadSubgraphInfo(const std::string &filename,
                          const std::string &absPath);
    const std::map<std::string, osg_graph_viz::NodeInfo>& getNodeInfoMap() {return infoMap;}
    bool groupNodes(unsigned long groupNodeId, unsigned long nodeId) {return false;}
    void importSmurf(std::string filename);
    void createInputPortsForSelection(std::list<osg::ref_ptr<osg_graph_viz::Node> > selectedNodes, double portFontSize, double headerFontSize,
                                      bool usePortNames=true);
    void createOutputPortsForSelection(std::list<osg::ref_ptr<osg_graph_viz::Node> > selectedNodes, double portFontSize, double headerFontSize);
    const std::string& getExternNodePath() {return externNodePath;}
    bool hasNode(const std::string &nodeName);
    bool hasNodeInfo(const std::string &type);
    bool hasConnection(const std::string &nodeName);
    void setModelInfo(configmaps::ConfigMap &map);
    configmaps::ConfigMap& getModelInfo();

  private:
    std::map<unsigned long, configmaps::ConfigMap> nodeMap, edgeMap;
    std::map<std::string, osg_graph_viz::NodeInfo> infoMap;
    std::string confDir, externNodePath;
    configmaps::ConfigMap modelInfo;

    bool getNode(const std::string &name, configmaps::ConfigMap **map);
    void handleMetaData(configmaps::ConfigMap &map);
    bool handleGenericProperties(configmaps::ConfigMap &chainNode,
                                 configmaps::ConfigItem *m);

  };
} // end of namespace bagel_bui

#endif // BAGEL_GUI_BAGEL_MODEL_HPP
