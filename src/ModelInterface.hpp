/**
 * \file ModelInterface.hpp
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief
 *
 * Version 0.1
 */

#ifndef BAGEL_GUI_MODEL_INTERFACE_HPP
#define BAGEL_GUI_MODEL_INTERFACE_HPP

#include <configmaps/ConfigMap.hpp>

class QWidget;

namespace osg_graph_viz {
  struct NodeInfo;
}

namespace bagel_gui {

  class BagelGui;

  class ModelInterface {
  public:
    ModelInterface(BagelGui *bagelGui) : bagelGui(bagelGui) {}
    virtual ~ModelInterface() {}

    virtual ModelInterface* clone() = 0;
    // load file or directory
    virtual bool addNode(unsigned long nodeId, configmaps::ConfigMap *node) = 0;
    virtual bool addEdge(unsigned long egdeId, configmaps::ConfigMap *edge) = 0;
    virtual bool addNode(unsigned long nodeId,
                         const configmaps::ConfigMap &node) = 0;
    virtual bool addEdge(unsigned long egdeId,
                         const configmaps::ConfigMap &edge) = 0;
    virtual bool hasEdge(configmaps::ConfigMap *edge) = 0;
    virtual bool hasEdge(const configmaps::ConfigMap &edge) = 0;
    virtual bool updateNode(unsigned long nodeId,
                            configmaps::ConfigMap node) = 0;
    virtual bool updateEdge(unsigned long egdeId,
                            configmaps::ConfigMap edge) = 0;
    virtual void preAddNode(unsigned long nodeId) = 0;
    virtual bool removeNode(unsigned long nodeId) = 0;
    virtual bool removeEdge(unsigned long edgeId) = 0;
    virtual bool loadSubgraphInfo(const std::string &filename,
                                  const std::string &absPath) = 0;
    virtual std::map<unsigned long, std::vector<std::string> > getCompatiblePorts(unsigned long nodeId, std::string outPortName) = 0;
    virtual bool handlePortCompatibility() = 0;
    virtual const std::map<std::string, osg_graph_viz::NodeInfo>& getNodeInfoMap() = 0;
    virtual bool groupNodes(unsigned long groupNodeId, unsigned long nodeId) = 0;

    virtual void displayWidget( QWidget *pParent ) {};
    virtual void setModelInfo(configmaps::ConfigMap &map) = 0;
    virtual configmaps::ConfigMap& getModelInfo() = 0;

  protected:
    BagelGui *bagelGui;
  };
} // end of namespace bagel_bui

#endif // BAGEL_GUI_MODEL_INTERFACE_HPP
