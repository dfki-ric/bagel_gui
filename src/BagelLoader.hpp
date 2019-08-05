/**
 * \file BagelLoader.hpp
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief Base class for node loader
 *
 * Version 0.1
 */

#include "NodeLoader.hpp"

#ifndef BAGEL_GUI_BAGEL_LOADER_HPP
#define BAGEL_GUI_BAGEL_LOADER_HPP

namespace bagel_gui {

  class BagelGui;
  
  class BagelLoader : public NodeLoader {
  public:
    BagelLoader(BagelGui *bagelGui) : NodeLoader(bagelGui) {}
    virtual ~BagelLoader() {}

    // load file or directory
    void loadNodeInfo(const std::string &filename);
    void load(const std::string &filename);
    void load(configmaps::ConfigMap &map, std::string loadPath,
              bool reload=false);
    void save(const configmaps::ConfigMap &map, const std::string &filename);
    void exportCnd(const configmaps::ConfigMap &map,
		   const std::string &filename);
    void handlePotentialLibraryChanges(configmaps::ConfigItem *node,
                                       std::string nodeName,
                                       osg_graph_viz::NodeInfo *info);
  };
} // end of namespace bagel_bui

#endif // BAGEL_GUI_BAGEL_LOADER_HPP
