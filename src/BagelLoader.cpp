/**
 * \file BagelLoader.cpp \author Malte Langosz (malte.langosz@dfki.de)
 * \brief Base class for node loader
 *
 * Version 0.1
 */

#include "BagelGui.hpp"
#include "BagelLoader.hpp"
#include <osg_graph_viz/Node.hpp>
#include <mars_utils/misc.h>
#include <dirent.h>
#include <QDir>

namespace bagel_gui {

  using namespace configmaps;

  // load file or directory
  void BagelLoader::loadNodeInfo(const std::string &filename) {
  }

  void BagelLoader::load(const std::string &filename) {
    ConfigMap map = ConfigMap::fromYamlFile(filename);
    std::string loadPath = mars::utils::getPathOfFile(filename);
    if(loadPath[loadPath.size()-1] != '/') loadPath.append("/");
    load(map, mars::utils::getPathOfFile(filename));
  }

  void BagelLoader::load(ConfigMap &map, std::string loadPath, bool reload) {
    ConfigVector::iterator it, it2;
    unsigned long nextOrderNumber = 1;

    QDir dir(QString::fromStdString(loadPath));
    std::string model = "bagel";
    std::string externNodePath;
    if(map.hasKey("model")) {
      model << map["model"];
    }
    if(map.hasKey("externNodePath")) {
      externNodePath << map["externNodePath"];
    }
    if(!reload) {
      bagelGui->setModel(model);
      if(!externNodePath.empty()) {
        bagelGui->setExternNodePath(loadPath, externNodePath);
      }
    }
    for(auto it: map["nodes"]) {
      try {
        osg_graph_viz::NodeInfo info;
        info.redrawEdges = false;
        unsigned long id = 0;
        if(it.hasKey("id")) {
          id = it["id"];
        }

        //int type = getNodeType(it->children["type"].getString());
        std::string type = it["type"];
        info.numInputs = 0;
        info.numOutputs = 0;

        if(type == "INPUT") {
          info.numOutputs = 1;
          if(it.hasKey("outputs")) {
            if(trim(it["outputs"][0]["name"].getString()) == "") {
              it["outputs"][0]["name"] = "out1";
            }
          }
          else {
            it["outputs"][0]["name"] = "out1";
          }
        }
        else if(type == "OUTPUT") {
          for(it2 = it["inputs"].begin();
              it2!=it["inputs"].end(); ++it2) {
            ++info.numInputs;
            if(!it2->hasKey("name") || trim((*it2)["name"].getString()) == "") {
              std::stringstream ss;
              ss << "in" << info.numInputs;
              (*it2)["name"] = ss.str();
            }
            //int mergeType = getMergeType(it2->children["type"].getString());
            char text[25];
            sprintf(text, "input %d", info.numInputs);
          }
        }
        else if(type == "EXTERN") {
          std::string externName = it["extern_name"];
          osg_graph_viz::NodeInfo externInfo = bagelGui->getNodeInfo(externName);
          info.numOutputs = externInfo.numOutputs;
          info.numInputs = externInfo.numInputs;

          if(!reload) {
            handlePotentialLibraryChanges(&it, externName, &info);
            fprintf(stderr, "%s: in / out: %d / %d\n", externName.c_str(), info.numInputs, info.numOutputs);
          }
        }
        else if(type == "SUBGRAPH") {
          // get the plane subgraph name and its relative path
          std::string subName = it["subgraph_name"];
          if(!reload) {
            std::string relPath = mars::utils::getPathOfFile(subName);
            mars::utils::removeFilenamePrefix(&subName);

            // assemble the absolute path
            QString qPath = dir.absoluteFilePath(QString::fromStdString(relPath));
            std::string absPath = QDir::cleanPath(qPath).toStdString();
            if(absPath[absPath.size()-1] != '/') absPath.append("/");

            bagelGui->addSubgraphInfo(subName, absPath);
            it["path"] = absPath;
            it["subgraph_name"] = subName;
          }
          osg_graph_viz::NodeInfo subInfo = bagelGui->getNodeInfo(subName);
          info.numOutputs = subInfo.numOutputs;
          info.numInputs = subInfo.numInputs;
          // for backward compatibility create on output if none exists
          if(!it.hasKey("outputs")) {
            it["outputs"][0]["name"] = "out1";
          }
          if(!reload) {
            handlePotentialLibraryChanges(&it, subName, &info);
            fprintf(stderr, "%s: in / out: %d / %d\n", subName.c_str(), info.numInputs, info.numOutputs);
          }
        }
        else {
          info.numOutputs = 0;
          for(it2 = it["inputs"].begin();
              it2!=it["inputs"].end(); ++it2) {
            ++info.numInputs;
            if(!it2->hasKey("name") || trim((*it2)["name"].getString()) == "") {
              std::stringstream ss;
              ss << "in" << info.numInputs;
              (*it2)["name"] = ss.str();
            }
            //int mergeType = getMergeType(it2->children["type"].getString());
            //char text[25];
            //sprintf(text, "input %d", info.numInputs);
          }
          // for backward compatibility create on output if none exists
          if(model == "bagel" && !it.hasKey("outputs")) {
            it["outputs"][0]["name"] = "out1";
          }
          for(it2 = it["outputs"].begin();
              it2!=it["outputs"].end(); ++it2) {
            ++info.numOutputs;
            if(!it2->hasKey("name") || trim((*it2)["name"].getString()) == "") {
              std::stringstream ss;
              ss << "out" << info.numOutputs;
              (*it2)["name"] = ss.str();
            }
            //int mergeType = getMergeType(it2->children["type"].getString());
            //char text[25];
            //sprintf(text, "input %d", info.numInputs);
          }
        }
        info.map = it;
        info.map["order"] = nextOrderNumber++;

        if(it.hasKey("pos")) {
          double x = it["pos"]["x"];
          double y = it["pos"]["y"];
          bagelGui->addNode(&info, x, y, &id, true, reload);
        }
        else {
          bagelGui->addNode(&info, 0, 0, &id, true, reload);
        }

        if(!reload) {
          //fprintf(stderr, "load node: %lu %s\n", id, info.map["name"].c_str());
        }
      } catch (const std::exception& e) {
        fprintf(stderr, "BagelLoader: Error loading node: %s\n",
                it["name"].getString().c_str());
        std::cerr << e.what() << std::endl;
      }
    }

    // load the descriptions
    for(auto it: map["descriptions"]) {
      try {
        osg_graph_viz::NodeInfo info;

        unsigned long id = 0;
        if(it.hasKey("id")) {
          id = it["id"];
        }

        //int type = getNodeType(it->children["type"][0].getString());
        std::string type = (std::string)it["type"];
        info.numInputs = 0;
        info.numOutputs = 0;

        info.map = it;
        info.map["order"] = nextOrderNumber++;

        if(it.hasKey("pos")) {
          double x = it["pos"]["x"];
          double y = it["pos"]["y"];
          bagelGui->addNode(&info, x, y, &id, true, reload);
        }
        else {
          bagelGui->addNode(&info, 0, 0, &id, true, reload);
        }

        if(!reload) {
          fprintf(stderr, "load description: %lu %s\n", id, info.map["name"].c_str());
        }
      } catch (const std::exception& e) {
        fprintf(stderr, "BagelLoader: Error loading description: %s\n",
                it["name"].getString().c_str());
        std::cerr << e.what() << std::endl;
      }
    }

    // load the descriptions
    for(auto it: map["meta"]) {
      try {
        osg_graph_viz::NodeInfo info;
        unsigned long id = it["id"];

        //int type = getNodeType(it->children["type"][0].getString());
        std::string type = (std::string)it["type"];
        info.numInputs = 0;
        info.numOutputs = 0;

        info.map = it;
        info.map["order"] = nextOrderNumber++;

        if(it.hasKey("pos")) {
          double x = it["pos"]["x"];
          double y = it["pos"]["y"];
          bagelGui->addNode(&info, x, y, &id, true, reload);
        }
        else {
          bagelGui->addNode(&info, 0, 0, &id, true, reload);
        }
        if(!reload) {
          fprintf(stderr, "load meta: %lu %s\n", id, info.map["name"].c_str());
        }
      } catch (const std::exception& e) {
        fprintf(stderr, "BagelLoader: Error loading meta: %s\n",
                it["name"].getString().c_str());
        std::cerr << e.what() << std::endl;
      }
    }

    // load the edges
    for(auto it: map["edges"]) {
      // handle backwards compatibility
      ConfigMap &edge = it;
      try {
        if(!edge.hasKey("fromNode") && edge.hasKey("fromNodeId")) {
          edge["fromNode"] = bagelGui->getNodeName(edge["fromNodeId"]);
          edge.erase("fromNodeId");
        }
        if(!edge.hasKey("toNode") && edge.hasKey("toNodeId")) {
          edge["toNode"] = bagelGui->getNodeName(edge["toNodeId"]);
          edge.erase("toNodeId");
        }
        if(!edge.hasKey("fromNodeOutput") && edge.hasKey("fromNodeOutputIdx")) {
          edge["fromNodeOutput"] = bagelGui->getOutPortName((std::string)edge["fromNode"], edge["fromNodeOutputIdx"]);
          edge.erase("fromNodeOutputIdx");
        }
        if(!edge.hasKey("toNodeInput") && edge.hasKey("toNodeInputIdx")) {
          edge["toNodeInput"] = bagelGui->getInPortName((std::string)edge["toNode"], edge["toNodeInputIdx"]);
          edge.erase("toNodeInputIdx");
        }
        bagelGui->addEdge(edge, reload);
      } catch (const std::exception& e) {
        fprintf(stderr, "BagelLoader: Error add edge: %s %s %s %s\n", edge["fromNode"].getString().c_str(),edge["fromNodeOutput"].getString().c_str(),edge["toNode"].getString().c_str(),edge["toNodeInput"].getString().c_str());
        std::cerr << e.what() << std::endl;
      }
    }

    if(!reload) {
      fprintf(stderr, "load completed\n");
    }
  }

  void BagelLoader::handlePotentialLibraryChanges(ConfigItem *node,
                                                  std::string nodeName,
                                                  osg_graph_viz::NodeInfo *info) {
    osg_graph_viz::NodeInfo nodeInfo = bagelGui->getNodeInfo(nodeName);
    info->numOutputs = nodeInfo.numOutputs;

    ConfigMap libMap = nodeInfo.map;
    size_t numPorts;
    std::string type[] = {"inputs", "outputs"};
    //fprintf(stderr, "handle potential: \n");
    info->redrawEdges = false;
    for(int t = 0; t < 2; ++t) {
      // 1.a) check the number of ports
      if(t == 0){
        numPorts = info->numInputs;
      }else{
        numPorts = info->numOutputs;
      }
      if(node->hasKey(type[t])) {
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
        fprintf(stderr, "%s needs redraw\n", nodeName.c_str());

        ConfigMap node_copy = *node;
        ConfigVector &v = (*node)[type[t]];
        v.clear();
        for(size_t i=0; i<numPorts; ++i) {
          bool found = false;
          std::string portName = libMap[type[t]][i]["name"];
          fprintf(stderr, "to check: %s\n", portName.c_str());
          // check if the port was existent before too get its metadata
          size_t j;
          for(j=0; j<node_copy[type[t]].size(); ++j) {
            if(std::string(node_copy[type[t]][j]["name"]) == portName){
              fprintf(stderr, "found: %s\n", node_copy[type[t]][j]["name"].c_str());
              found = true;
              break;
            }
          }

          if(found){
            (*node)[type[t]][i]["name"] = portName;
            if(t==0){ // these fields are only used by inputs
              (*node)[type[t]][i]["type"] = node_copy[type[t]][j]["type"];
              (*node)[type[t]][i]["bias"] = node_copy[type[t]][j]["bias"];
              (*node)[type[t]][i]["default"] = node_copy[type[t]][j]["default"];
            }
          }else{
            (*node)[type[t]][i]["name"] = portName;
            if(t==0){ // these fields are only used by inputs
              (*node)[type[t]][i]["type"] = "SUM";
              (*node)[type[t]][i]["bias"] = 0.0;
              (*node)[type[t]][i]["default"] = 0.0;
            }
          }
        }
      }
    }
    return;
  }

  void BagelLoader::save(const configmaps::ConfigMap &map_,
                         const std::string &filename) {
    ConfigMap map = map_;
    std::list<ConfigMap> orderedNodes;
    std::list<ConfigMap>::iterator listIt;
    // handle file path and node order
    ConfigVector::iterator it = map["nodes"].begin();
    for(; it!=map["nodes"].end(); ++it) {
      ConfigMap &node = *it;

      // if a temporare absolute paths is given, use it to store
      // the relative path with the node type
      if(node.hasKey("path")) {
        QDir dir(QString::fromStdString(mars::utils::getPathOfFile(filename)));
        std::string absPath = node["path"];
        std::string relPath = dir.relativeFilePath(QString::fromStdString(absPath)).toStdString();
        if(relPath.size()>0) if(relPath[relPath.size()-1] != '/') relPath.append("/");
        if(relPath == "./") relPath = "";
        // and add the relative path to the subgraph name
        node["subgraph_name"] = relPath + (std::string)node["subgraph_name"];

        // delete the path information since it is not needed anymore
        node.erase("path");
      }

      // add the node map at the correct place
      unsigned long order = node["order"];
      for(listIt=orderedNodes.begin(); listIt!=orderedNodes.end();
          ++listIt) {
        if((unsigned long)(*listIt)["order"] > order) {
          break;
        }
      }

      orderedNodes.insert(listIt, node);
    }

    ((ConfigVector&)map["nodes"]).clear();
    for(listIt=orderedNodes.begin(); listIt!=orderedNodes.end();
        ++listIt) {
      map["nodes"] += *listIt;
      //fprintf(stderr, "nodes: %lu\n", conf["nodes"].size());
    }
    map.toYamlFile(filename);
  }

  void trimMap(ConfigItem &item) {
    if(item.isMap()) {
      ConfigMap::iterator it = item.beginMap();
      while(it != item.endMap()) {
        if(it->second.isAtom()) {
          std::string value = trim(it->second.toString());
          if(value.empty()) {
            item.erase(it);
            it = item.beginMap();
          }
          else {
            ++it;
          }
        }
        else if(it->second.isMap() || it->second.isVector()) {
          // todo: handle empty map
          trimMap(it->second);
          if(it->second.size() == 0) {
            item.erase(it);
            it = item.beginMap();
          }
          else {
            ++it;
          }
        }
        else {
          item.erase(it);
          it = item.beginMap();
        }
      }
    }
    else if(item.isVector()) {

      ConfigVector::iterator it = item.begin();
      while(it!=item.end()) {
        if(it->isAtom()) {
          std::string value = trim(it->toString());
          if(value.empty()) {
            item.erase(it);
            it = item.begin();
          }
          else {
            ++it;
          }
        }
        else if(it->isMap() || it->isVector()) {
          trimMap(*it);
          if(it->size() == 0) {
            item.erase(it);
            it = item.begin();
          }
          else {
            ++it;
          }
        }
        else {
          item.erase(it);
          it = item.begin();
        }
      }
    }
  }

  void BagelLoader::exportCnd(const configmaps::ConfigMap &map_,
                              const std::string &filename) {
    ConfigMap map = map_;
    ConfigMap output;
    std::list<ConfigMap>::iterator listIt;
    // handle file path and node order
    ConfigVector::iterator it = map["nodes"].begin();
    for(; it!=map["nodes"].end(); ++it) {
      ConfigMap &node = *it;
      if(node["type"].getString() == "software::Deployment") {
        std::string name = node["name"];
        // remove domain namespace
        name = name.substr(10);
        output["deployments"][name]["deployer"] = "orogen";
        output["deployments"][name]["process_name"] = "some_random_name";
        output["deployments"][name]["hostID"] = "localhost";
      }
      else {
        std::string name = node["name"];
        // remove domain namespace
        name = name.substr(10);
        ConfigItem m(node["data"]);
        trimMap(m);
        std::string type = node["type"];
        // remove domain namespace
        m["type"] = type.substr(10);
        output["tasks"][name] = m;
        if(node.hasKey("parentName")) {
          std::string parent = node["parentName"].getString();
          output["deployments"][parent]["taskList"][name] = name;
        }
      }
    }
    it = map["edges"].begin();
    int i=0;
    for(; it!=map["edges"].end(); ++it, ++i) {
      ConfigMap m;
      ConfigMap &edge = *it;
      if(edge.hasKey("transport")) {
        m["transport"] = edge["transport"];
      }
      if(edge.hasKey("type")) {
        m["type"] = edge["type"];
      }
      if(edge.hasKey("size")) {
        m["size"] = edge["size"];
      }
      std::string name = edge["fromNode"];
      // remove domian namespace
      m["from"]["task_id"] = name.substr(10);;
      m["from"]["port_name"] = edge["fromNodeOutput"];
      // remove domian namespace
      name << edge["toNode"];
      m["to"]["task_id"] = name.substr(10);
      m["to"]["port_name"] = edge["toNodeInput"];
      char buffer[100];
      // todo: use snprintf
      sprintf(buffer, "%d", i);
      output["connections"][std::string(buffer)] = m;
    }
    output.toYamlFile(filename);
  }

} // end of namespace bagel_bui
