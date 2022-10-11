/**
 * \file BagelModel.cpp
 * \author Malte Langosz (malte.langosz@dfki.de)
 * \brief  Model implementation for Bagel
 *
 * Version 0.1
 */

#include "BagelGui.hpp"
#include "BagelModel.hpp"
#include <osg_graph_viz/Node.hpp>
#include <mars/utils/misc.h>
#include <dirent.h>
#include <QDir>

namespace bagel_gui {

  using namespace configmaps;

  BagelModel::BagelModel(BagelGui *bagelGui) : ModelInterface(bagelGui) {
    confDir = bagelGui->getConfigDir();
    ConfigMap config = ConfigMap::fromYamlFile(confDir+"/config_default.yml", true);
    if(mars::utils::pathExists(confDir+"/config.yml")) {
      config.append(ConfigMap::fromYamlFile(confDir+"/config.yml", true));
    }

    ConfigVector::iterator it = config["bagel_node_definitions"].begin();
    for(; it!=config["bagel_node_definitions"].end(); ++it) {
      std::string filename = (*it);
      if(filename[0] == '/') {
        loadNodeInfo(filename);
      }
      else {
        loadNodeInfo(confDir+"/"+filename);
      }
    }
  }

  ModelInterface* BagelModel::clone() {
    return new BagelModel(bagelGui);
  }

  // load file or directory
  void BagelModel::loadNodeInfo(const std::string &filename) {
    ConfigMap node_config = ConfigMap::fromYamlFile(filename, true);
    for(auto it: node_config["nodes"]) {
      ConfigMap map = it;
      try {
        osg_graph_viz::NodeInfo info;
        if(map.find("inputs") == map.end()) {
          info.numInputs = 0;
        } else {
          info.numInputs = map["inputs"].size();
        }
        if(map.find("outputs") == map.end()) {
          info.numOutputs = 0;
        } else {
          info.numOutputs = map["outputs"].size();
        }
        info.map = map;
        info.map["name"] = "";
        info.type = (std::string)map["name"];
        if(infoMap.find(info.type) != infoMap.end()){
          fprintf(stderr, "Warning '%s' was already loaded and is ignorded now\n", info.type.c_str());
          continue;
        }
        infoMap[info.type] = info;
        //bagelGui->addNodeType(info);
      } catch (const std::exception& e) {
        fprintf(stderr, "BagelModel: error loading node %s from file: %s\n",
                it["name"].getString().c_str(), filename.c_str());
        std::cerr << e.what() << std::endl;
      }
    }
    if(node_config.hasKey("subgraphs")) {
      for(auto it: node_config["subgraphs"]) {
        try {
          std::string filename = it.getString();
          std::string absPath = mars::utils::getPathOfFile(filename);
          if(absPath[absPath.size()-1] != '/') absPath.append("/");
          mars::utils::removeFilenamePrefix(&filename);
          loadSubgraphInfo(filename, absPath);
        } catch (const std::exception& e) {
          fprintf(stderr, "BagelModel: error loading subgraph %s from file: %s\n",
                  it.getString().c_str(), filename.c_str());
          std::cerr << e.what() << std::endl;
        }
      }
    }
    // load the installed extern nodes (path specified by config file)
    if(node_config.hasKey("ExternNodesPath")) {
      try {
        std::string file = node_config["ExternNodesPath"];
        if(file[0] != '/') {
          file = confDir+"/"+file;
        }
        getExternNodes(file);
        externNodePath = "";
      } catch (const std::exception& e) {
        fprintf(stderr, "BagelModel: Error loading extern nodes from: %s\n", filename.c_str());
        std::cerr << e.what() << std::endl;
      }
    }
  }

  void BagelModel::getExternNodes(std::string path, std::string rootPath) {
    externNodePath = path;
    if(!rootPath.empty()) {
      if (rootPath.find_last_of("/") != rootPath.size() - 1)
        rootPath += "/";
      path = rootPath + path;
    }
    // if there is no slash at the end of the path, we'll add it.
    if (path.find_last_of("/") != path.size() - 1)
      path += "/";
    //fprintf(stderr, "%s\n", path.c_str());
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir (path.c_str())) != NULL) {
      // go through all entities
      while ((ent = readdir (dir)) != NULL) {
        std::string file = ent->d_name;

        if (file.find(".yml", file.size() - 4, 4) != std::string::npos) {
          // try to load the yaml-file
          ConfigMap externMap = ConfigMap::fromYamlFile(path + file);
          addExternNode(externMap);
        } else if (file.find(".", 0, 1) != std::string::npos) {
          // skip ".*"
        } else {
          // go into the next dir
          getExternNodes(path + file + "/");
        }
      }
      closedir (dir);
    } else {
      // this is not a directory
      fprintf(stderr, "Specified path '%s' is not a valid directory\n",
              path.c_str());
    }
    return;
  }

  void BagelModel::addExternNode(ConfigMap externMap) {
    std::string name = std::string(externMap["name"]);
    // ignore files which have not the name tag, i.e. which are no extern nodes
    if(name == "") return;
    osg_graph_viz::NodeInfo info;

    ConfigMap map;
    map["name"] = name;
    map["extern_name"] = name;
    map["type"] = "EXTERN";

    if(externMap.hasKey("inputs")) {
      info.numInputs = externMap["inputs"].size();
      for(size_t i=0; i<externMap["inputs"].size(); ++i) {
        map["inputs"][i]["bias"] = 0.0;
        map["inputs"][i]["default"] = 0.0;
        map["inputs"][i]["idx"] = (unsigned long)i;
        map["inputs"][i]["type"] = "SUM";
        map["inputs"][i]["name"] = externMap["inputs"][i]["name"];
      }
    }

    if(externMap.hasKey("outputs")) {
      info.numOutputs = externMap["outputs"].size();
      for(size_t i=0; i<externMap["outputs"].size(); ++i) {
        map["outputs"][i]["name"] = externMap["outputs"][i]["name"];
      }
    }

    info.map = map;
    info.type = name;
    if(infoMap.find(info.type) != infoMap.end()){
      fprintf(stderr, "Warning '%s' was already loaded and is ignorded now\n", info.type.c_str());
      return;
    }
    infoMap[info.type] = info;
  }

  void BagelModel::handlePotentialLibraryChanges(ConfigVector::iterator node,
                                                  std::string nodeName,
                                                  osg_graph_viz::NodeInfo *info) {
    //return;
    osg_graph_viz::NodeInfo nodeInfo = bagelGui->getNodeInfo(nodeName);
    info->numOutputs = nodeInfo.numOutputs;

    ConfigMap libMap = nodeInfo.map;
    size_t numPorts;
    std::string type[] = {"inputs", "outputs"};
    //fprintf(stderr, "handle potential: \n");
    for(int t = 0; t < 2; ++t) {
      info->redrawEdges = false;
      // 1.a) check the number of ports
      if(t == 0){
        numPorts = info->numInputs;
      }else{
        numPorts = info->numOutputs;
      }
      if(node->hasKey(type[t]) && (*node)[type[t]].isVector()) {
        //fprintf(stderr, "%lu %lu\n", numPorts, (*node)[type[t]].size());
        if(numPorts == (*node)[type[t]].size()){

          // 1.b) check if the ports are identical
          ConfigVector::iterator curPortIt = (*node)[type[t]].begin();
          ConfigVector::iterator libPortIt = libMap[type[t]].begin();
          for(; curPortIt != (*node)[type[t]].end(); ++curPortIt, ++libPortIt) {
            if(std::string((*curPortIt)["name"]) != std::string((*libPortIt)["name"])){
              info->redrawEdges = true;
              break;
            }
          }
        }else{ // port number mismatch
          info->redrawEdges = true;
        }
      }

      /* 2. if the node has changed, replace the current node with the library node
         - but keep the properties of the existent ports */
      if(info->redrawEdges){
        //fprintf(stderr, "%s needs redraw\n", nodeName.c_str());

        ConfigMap node_copy = *node;
        ConfigVector &v = (*node)[type[t]];
        v.clear();
        for(size_t i=0; i<numPorts; ++i) {
          bool found = false;
          std::string portName = libMap[type[t]][i]["name"];
          //fprintf(stderr, "to check: %s\n", portName.c_str());
          // check if the port was existent before too get its metadata
          size_t j;
          for(j=0; j<node_copy[type[t]].size(); ++j) {
            if(std::string(node_copy[type[t]][j]["name"]) == portName){
              //fprintf(stderr, "found: %s\n", node_copy[type[t]][j]["name"].c_str());
              found = true;
              break;
            }
          }

          if(found){
            (*node)[type[t]][i]["name"] = portName;
            (*node)[type[t]][i]["type"] = node_copy[type[t]][j]["type"];
            (*node)[type[t]][i]["bias"] = node_copy[type[t]][j]["bias"];
            (*node)[type[t]][i]["default"] = node_copy[type[t]][j]["default"];
          }else{
            (*node)[type[t]][i]["name"] = portName;
            (*node)[type[t]][i]["type"] = "SUM";
            (*node)[type[t]][i]["bias"] = 0.0;
            (*node)[type[t]][i]["default"] = 0.0;
          }
        }
      }
    }
    return;
  }

  bool BagelModel::loadSubgraphInfo(const std::string &filename,
                                    const std::string &absPath) {
    if(infoMap.find(filename) != infoMap.end()) return false;
    ConfigMap subgraph = ConfigMap::fromYamlFile(absPath + filename);
    ConfigVector::iterator it;
    osg_graph_viz::NodeInfo info;
    std::vector<std::string> inNames, outNames;
    info.numInputs = 0;
    info.numOutputs = 0;
    if(subgraph.find("nodes") == subgraph.end()) {
      fprintf(stderr, "information for subgraph '%s' not correct\n",
              filename.c_str());
      return false;
    }
    for(it=subgraph["nodes"].begin(); it!=subgraph["nodes"].end(); ++it) {
      if((std::string)(*it)["type"] == "INPUT") {
        info.numInputs++;
        inNames.push_back((std::string)(*it)["name"]);
      }
      else if((std::string)(*it)["type"] == "OUTPUT") {
        info.numOutputs++;
        outNames.push_back((std::string)(*it)["name"]);
      }
    }
    ConfigMap map;
    for(int i=0; i<info.numInputs; ++i) {
      map["inputs"][i]["bias"] = 0.0;
      map["inputs"][i]["default"] = 0.0;
      map["inputs"][i]["idx"] = i;
      map["inputs"][i]["type"] = "SUM";
      map["inputs"][i]["name"] = inNames[i];
    }
    for(int i=0; i<info.numOutputs; ++i) {
      map["outputs"][i]["name"] = outNames[i];
    }
    if(subgraph.hasKey("meta")) {
      map["meta"] = subgraph["meta"];
    }

    // fill all data
    map["name"] = "";
    map["type"] = "SUBGRAPH";
    map["subgraph_name"] = filename;
    map["path"] = absPath;
    info.map = map;

    // just add the subgraph when its new
    //fprintf(stderr, "adding '%s' to widget\n", filename.c_str());
    infoMap[filename] = info;
    return true;
  }

  void BagelModel::importSmurf(std::string filename) {
    std::string path = mars::utils::getPathOfFile(filename);
    ConfigMap map = ConfigMap::fromYamlFile(filename);
    ConfigMap motorMap;
    ConfigMap chainMap;
    if(map.hasKey("files")) {
      std::string file;
      ConfigVector::iterator it = map["files"].begin();
      for(; it!=map["files"].end(); ++it) {
        file << *it;
        if(file.find("motors") != std::string::npos) {
          motorMap = ConfigMap::fromYamlFile(path+"/"+file);
        }
        else if(file.find("chains") != std::string::npos) {
          chainMap = ConfigMap::fromYamlFile(path+"/"+file);
        }
      }
    }
    if(motorMap.hasKey("motors")) {
      ConfigVector::iterator it = motorMap["motors"].begin();
      double step = 22.0;
      double n=(motorMap["motors"].size()*1.)*step;
      for(; it!=motorMap["motors"].end(); ++it) {
        std::string motorName = (*it)["name"].getString()+"/des_angle";
        bool found = false;
        std::map<unsigned long, configmaps::ConfigMap>::iterator nt;
        for(nt=nodeMap.begin(); nt!=nodeMap.end(); ++nt) {
          if(nt->second["name"].getString() == motorName) {
            found = true;
            break;
          }
        }
        if(!found) {
          bagelGui->addNode("OUTPUT", motorName, 500.0, n);
        }
        n -= step;
      }
    }
    if(chainMap.hasKey("chains")) {
      ConfigVector::iterator it = chainMap["chains"].begin();
      for(; it!=chainMap["chains"].end(); ++it) {
        ConfigMap &map = *it;
        if(!map.hasKey("software")) continue;
        std::string controller = map["software"][0]["reference"];
        std::string chainName = map["name"];
        ConfigMap *chainNodePtr;
        bool found = getNode(chainName, &chainNodePtr);
        if(!found) {
          bagelGui->addNode(controller, chainName, 500.0, 0);
          found = getNode(chainName, &chainNodePtr);
        }
        // we were not able to create the node
        if(!found) continue;
        ConfigMap &chainNode = *chainNodePtr;

        // handle input parameter
        if(handleGenericProperties(chainNode, map["properties"])) {
          bagelGui->updateNodeMap(chainName, chainNode);
        }
        ConfigVector::iterator vt = map["actuators"].begin();
        std::vector<std::string>::iterator lt;
        size_t i=0;
        for(; vt!=map["actuators"].end(); ++vt) {
          if(i>=chainNode["outputs"].size()) break;
          std::string motorName = vt->getString()+"/des_angle";
          ConfigMap edgeInfo;
          edgeInfo["fromNode"] = chainName;
          edgeInfo["fromNodeOutput"] = chainNode["outputs"][i++]["name"];
          edgeInfo["toNode"] = motorName;
          edgeInfo["toNodeInput"] = "in1";
          edgeInfo["weight"] = 1.0;
          edgeInfo["ignore_for_sort"] = 0;

          if(!hasConnection(motorName)) {
            bagelGui->addEdge(edgeInfo);
          }
        }
      }
    }
  }

  bool BagelModel::getNode(const std::string &name, configmaps::ConfigMap **map) {
    //bool found = false;
    std::map<unsigned long, configmaps::ConfigMap>::iterator it;
    for(it=nodeMap.begin(); it!=nodeMap.end(); ++it) {
      if(it->second["name"].getString() == name) {
        *map = &(it->second);
        return true;
      }
    }
    return false;
  }

  bool BagelModel::addNode(unsigned long nodeId, configmaps::ConfigMap *node) {
    return addNode(nodeId, *node);
  }

  bool BagelModel::addNode(unsigned long nodeId,
                           const configmaps::ConfigMap &node) {
    ConfigMap map = node;
    std::string nodeName = map["name"].getString();
    std::map<unsigned long, configmaps::ConfigMap>::iterator nt;
    for(nt=nodeMap.begin(); nt!=nodeMap.end(); ++nt) {
      if(nt->second["name"].getString() == nodeName) {
        return false;
      }
    }
    nodeMap[nodeId] = map;
    return true;
  }

  // todo: pre is wrong, post instead
  void BagelModel::preAddNode(unsigned long nodeId) {
    // check if we have to add a dependency
    handleMetaData(nodeMap[nodeId]);
  }

  bool BagelModel::removeNode(unsigned long nodeId) {
    nodeMap.erase(nodeId);
    return true;
  }

  bool BagelModel::addEdge(unsigned long id, configmaps::ConfigMap *edge) {
    return addEdge(id, *edge);
  }

  bool BagelModel::addEdge(unsigned long id,
                           const configmaps::ConfigMap &edge) {
    edgeMap[id] = edge;
    return true;
  }

  bool BagelModel::hasEdge(configmaps::ConfigMap *edge) {
    ConfigMap &map = *edge;

    for (auto it=edgeMap.begin(); it!=edgeMap.end(); ++it) {
      if (it->second["fromNode"]       == map["fromNode"]       &&
          it->second["toNode"]         == map["toNode"]         &&
          it->second["fromNodeOutput"] == map["fromNodeOutput"] &&
          it->second["toNodeInput"]    == map["toNodeInput"]
         )
      {
        return true;
      }
    }
    return false;
  }

  bool BagelModel::hasEdge(const configmaps::ConfigMap &edge) {
    ConfigMap map = edge;
    return hasEdge(&map);
  }

  bool BagelModel::removeEdge(unsigned long id) {
    edgeMap.erase(id);
    return true;
  }

  bool BagelModel::updateEdge(unsigned long id, configmaps::ConfigMap edge) {
    // todo: bug here
    if(edgeMap.find(id) == edgeMap.end()) return true;
    edgeMap[id] = edge;
    return true;
  }

  // todo: don't create nodes if they already exists
  void BagelModel::createInputPortsForSelection(std::list<osg::ref_ptr<osg_graph_viz::Node> > selectedNodes,
                                                double portFontSize,
                                                double headerFontSize,
                                                bool usePortNames) {
    // go through all selected nodes
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    for(it=selectedNodes.begin(); it!=selectedNodes.end(); ++it) {
      osg_graph_viz::NodeInfo info = (*it)->getNodeInfo();
      // fprintf(stderr, "creating input ports for node %s\n",
      //        (*it)->getName().c_str());
      for(int idx = 0; idx < info.numInputs; ++idx){
        // create a port
        osg::Vec3 portPosition = (*it)->getInPortPos(idx);
        std::string name = (*it)->getInPortName(idx);
        if(!usePortNames) {
          name = (*it)->getName()+"/"+(*it)->getInPortName(idx);
        }
        //fprintf(stderr, "create: %s\n", name.c_str());
        // search if node already exists
        bool found = false;
        // first check if the port doesn't have a connection already
        bool createEdge = !(*it)->hasInPortConnection(idx);
        // then check if no default value is set for the port
        if(info.map["inputs"][idx].hasKey("defaultIsSet")){
          createEdge = !info.map["inputs"][idx]["defaultIsSet"];
        }
        found = !createEdge;
        // at last check if there is no node already
        std::map<unsigned long, configmaps::ConfigMap>::iterator nt;
        if(createEdge) {
          for(nt=nodeMap.begin(); nt!=nodeMap.end(); ++nt) {
            if(nt->second["name"].getString() == name) {
              found = true;
              break;
            }
          }
        }
        std::string outPortName;
        if(!found) {
          double xOffset = -200.0;
          double yOffset = std::max(2.0 * portFontSize - 2.0,
                                    headerFontSize + 2.0) / 2.0;
          bagelGui->addNode("INPUT", name, portPosition[0] + xOffset,
                            portPosition[1] + yOffset);
          outPortName = "out1";
        }
        else if(createEdge) {
          outPortName << nt->second["outputs"][0]["name"];
        }
        if(createEdge) {
          // create an edge
          ConfigMap edgeInfo;
          edgeInfo["fromNode"] = name;
          edgeInfo["fromNodeOutput"] = outPortName;
          edgeInfo["toNode"] = (*it)->getName();
          edgeInfo["toNodeInput"] = (*it)->getInPortName(idx);
          edgeInfo["weight"] = 1.0;
          edgeInfo["ignore_for_sort"] = 0;
          edgeInfo["decouple"] = false;
          edgeInfo["smooth"] = true;
          bagelGui->addEdge(edgeInfo);
        }
      }
    }
  }

    void BagelModel::createOutputPortsForSelection(std::list<osg::ref_ptr<osg_graph_viz::Node> > selectedNodes,
                                                   double portFontSize,
                                                   double headerFontSize) {
    // go through all selected nodes
    std::list<osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    for(it=selectedNodes.begin(); it!=selectedNodes.end(); ++it) {
      osg_graph_viz::NodeInfo info = (*it)->getNodeInfo();
      // fprintf(stderr, "creating input ports for node %s\n",
      //        (*it)->getName().c_str());
      for(int idx = 0; idx < info.numOutputs; ++idx){
        // create a port
        osg::Vec3 portPosition = (*it)->getOutPortPos(idx);
        std::string name = (*it)->getOutPortName(idx);
        name = (*it)->getName()+"/"+name;
        //fprintf(stderr, "create: %s\n", name.c_str());
        // search if node already exists
        bool found = false;
        // at last check if there is no node already
        std::map<unsigned long, configmaps::ConfigMap>::iterator nt;
        for(nt=nodeMap.begin(); nt!=nodeMap.end(); ++nt) {
          if(nt->second["name"].getString() == name) {
            found = true;
            break;
          }
        }
        std::string inPortName;
        if(!found) {
          double xOffset = 200.0;
          double yOffset = std::max(2.0 * portFontSize - 2.0,
                                    headerFontSize + 2.0) / 2.0;
          bagelGui->addNode("OUTPUT", name, portPosition[0] + xOffset,
                            portPosition[1] + yOffset);
          inPortName = "in1";
        }
        {
          // create an edge
          ConfigMap edgeInfo;
          edgeInfo["toNode"] = name;
          edgeInfo["toNodeInput"] = inPortName;
          edgeInfo["fromNode"] = (*it)->getName();
          edgeInfo["fromNodeOutput"] = (*it)->getOutPortName(idx);
          edgeInfo["weight"] = 1.0;
          edgeInfo["ignore_for_sort"] = 0;
          edgeInfo["decouple"] = false;
          edgeInfo["smooth"] = true;
          bagelGui->addEdge(edgeInfo);
        }
      }
    }
  }

  void BagelModel::handleMetaData(ConfigMap &map) {
    if(map.hasKey("meta")) {
      ConfigVector::iterator it = map["meta"].begin();
      for(; it!=map["meta"].end(); ++it) {
        if(it->hasKey("software")) {
          ConfigVector::iterator it2 = (*it)["software"].begin();
          for(; it2!=(*it)["software"].end(); ++it2) {
            std::string file = (*it2)["reference"];
            bool once = true;
            if(it2->hasKey("once")) {
              once = (*it2)["once"];
            }
            if(!once) {
              // add new node
            }
            else {
              ConfigMap *map2;
              std::string nodeName = file;
              mars::utils::removeFilenameSuffix(&nodeName);
              if(!getNode(nodeName, &map2)) {
                bagelGui->addNode(file, nodeName);
              }
              if(it2->hasKey("connect")) {
                ConfigVector::iterator it3 = (*it2)["connect"].begin();
                for(; it3!=(*it2)["connect"].end(); ++it3) {
                  ConfigMap edgeInfo;
                  edgeInfo["fromNode"] = nodeName;
                  edgeInfo["fromNodeOutput"] = (*it3)["from"].getString();
                  edgeInfo["toNode"] = map["name"].getString();
                  edgeInfo["toNodeInput"] = (*it3)["to"].getString();
                  edgeInfo["weight"] = 1.0;
                  edgeInfo["ignore_for_sort"] = 0;
                  edgeInfo["decouple"] = true;
                  bagelGui->addEdge(edgeInfo);
                }
              }
            }
          }
        }
      }
    }
  }

  bool BagelModel::handleGenericProperties(ConfigMap &chainNode, ConfigItem *item) {
    bool update = false;
    ConfigVector::iterator it = chainNode["inputs"].begin();
    for(; it!=chainNode["inputs"].end(); ++it) {
      ConfigItem *m = item;
      std::string name = (*it)["name"].getString();
      std::vector<std::string> split = mars::utils::explodeString('/',name);
      bool found3 = true;
      for(size_t i=0; i<split.size(); ++i) {
        if(m->hasKey(split[i])) {
          m = (*m)[split[i]];
        }
        else {
          found3 = false;
          break;
        }
      }
      if(found3) {
        update = true;
        (*it)["default"] = *m;
        (*it)["defaultIsSet"] = true;
      }
    }
    return update;
  }

  bool BagelModel::updateNode(unsigned long nodeId, configmaps::ConfigMap node) {
    if(nodeMap.find(nodeId) == nodeMap.end()) return false;
    nodeMap[nodeId] = node;
    return true;
  }

  bool BagelModel::hasConnection(const std::string &nodeName) {
    std::map<unsigned long, configmaps::ConfigMap>::iterator it;
    for(it=edgeMap.begin(); it!=edgeMap.end(); ++it) {
      if(it->second["fromNode"].getString() == nodeName) return true;
      if(it->second["toNode"].getString() == nodeName) return true;
    }
    return false;
  }

  bool BagelModel::hasNode(const std::string &nodeName) {
    std::map<unsigned long, configmaps::ConfigMap>::iterator nt;
    for(nt=nodeMap.begin(); nt!=nodeMap.end(); ++nt) {
      if(nt->second["name"].getString() == nodeName) {
        return true;
      }
    }
    return false;
  }

  bool BagelModel::hasNodeInfo(const std::string &type) {
    return (infoMap.find(type) != infoMap.end());
  }

  void BagelModel::setModelInfo(configmaps::ConfigMap &map) {
    modelInfo = map;
  }

  configmaps::ConfigMap& BagelModel::getModelInfo() {
    return modelInfo;
  }

} // end of namespace bagel_bui
