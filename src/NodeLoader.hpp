/**
 * \file NodeLoader.hpp
 * \author Malte (malte.langosz@dfki.de)
 * \brief Base class for node loader
 *
 * Version 0.1
 */

#ifndef BAGEL_GUI_NODE_LOADER_HPP
#define BAGEL_GUI_NODE_LOADER_HPP

#include <configmaps/ConfigMap.hpp>

namespace bagel_gui {

  class BagelGui;
  
  class NodeLoader {
  public:
    NodeLoader(BagelGui *bagelGui) : bagelGui(bagelGui) {}
    virtual ~NodeLoader() {}

    // load file or directory
    virtual void loadNodeInfo(const std::string &filename) = 0;
    virtual void load(const std::string &filename) = 0;
    virtual void load(configmaps::ConfigMap &map, std::string loadPath,
                      bool reload = false) = 0;
    virtual void save(const configmaps::ConfigMap &map,
                      const std::string &filename) = 0;
    virtual void exportCnd(const configmaps::ConfigMap &map,
			   const std::string &filename) = 0;
  protected:
    BagelGui *bagelGui;
  };
} // end of namespace bagel_bui

#endif // BAGEL_GUI_NODE_LOADER_HPP
