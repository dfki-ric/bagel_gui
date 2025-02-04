/**
 * \file PluginInterface.hpp
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief 
 *
 * Version 0.1
 */

#ifndef BAGEL_GUI_PLUGIN_INTERFACE_HPP
#define BAGEL_GUI_PLUGIN_INTERFACE_HPP

#include <string>
#include <vector>

namespace bagel_gui {

  class ModelInterface;

  class PluginInterface {
  public:
    PluginInterface() {}
    virtual ~PluginInterface() {}

    virtual void currentModelChanged(ModelInterface *model) {}
    virtual void nodeContextClicked(std::string name) {}
    virtual void edgeContextClicked(std::string name) {}
    virtual void inPortContextClicked(std::string name) {}
    virtual void outPortContextClicked(std::string name) {}
    virtual std::vector<std::string> getNodeContextStrings(const std::string &name) {
      return std::vector<std::string>();
    }
    virtual std::vector<std::string> getEdgeContextStrings(const std::string &name)
    {
      return std::vector<std::string>();
    }
    virtual std::vector<std::string> getInPortContextStrings(const std::string &nodeName, const std::string &portName) {
      return std::vector<std::string>();
    }
    virtual std::vector<std::string> getOutPortContextStrings(const std::string &nodeName, const std::string &portName) {
      return std::vector<std::string>();
    }
  };

} // end of namespace bagel_bui

#endif // BAGEL_GUI_PLUGIN_INTERFACE_HPP
