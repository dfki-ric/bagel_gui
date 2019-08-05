# bagel_gui

Bagel (Biologically inspired Graph-Based Language) is a cross-platform
graph-based dataflow language developed at the
[Robotics Innovation Center of the German Research Center for Artificial Intelligence (DFKI-RIC)](http://robotik.dfki-bremen.de/en/startpage.html)
and the [University of Bremen](http://www.informatik.uni-bremen.de/robotik/index_en.php).
It runs on (Ubuntu) Linux, Mac and Windows.

This package provides a graph GUI to visualize and edit Bagel graphs.
The software is developed in a more generic fashion allowing to extend/use
it for other graph models as well.

# General {#general}

The main user documentation of Bagel can be found at:
https://github.com/dfki-ric/bagel_wiki/wiki

The API documentation of `osg_graph_viz` can be build in the `doc`
sub-folder with the `make` command. The documentation is build into
the `doc/build` folder.

## Install

  Either see: https://git.hb.dfki.de/team-learning/package_definitions

  or ROCK users can simply import the bagel/package_set and execute autoproj build bagel/gui

## Configuration

  The bagel gui is more or less a generic graph gui and needs the following
  configuration file:

  - **libs.txt**: Used by the generic [gui_app][] to load the bagel_gui
                  possible configuration:

```
cfg_manager_gui
bagel_gui
```

  -  **node_config.yml**: Used to define the path to extern nodes and to load the possible node definitions for the
                          graph. possible configuration:

```
extern_nodes_path: "/install/share/bagel/master/extern_nodes/"
nodes:
  - name: input
    type: INPUT
    outputs: [name: out1]
  - name: pipe
    type: PIPE
    inputs: [name: in1, name: in2]
    outputs: [name: out1, name: out2]
  - name: output
    type: OUTPUT
    inputs: [name: in1]
```

## Execute

  To execute the gui start the [gui_app][] within the configuration folder
  where the two configuration files are located.

  The bagel gui installs a configuration for editing bagel graph into:

  install/configuration/bagel_gui

[gui_app]: https://github.com/rock-simulation/mars/tree/master/common/gui/gui_app

## Todo:
  - history causes creation of a port in description node which causes error during save
  - orthogonal edges are loaded incorrect (sometimes)
  - decoupling edges sometimes produces a visible left over of old edge
  - change edge types
  - move node while creating new edge
  - keep orthogonal edges correct while moving nodes
  - fold icon
  - median merge icon
  - handle port names correctly (no edit of port node names)
  - error console
  - open subgraph in new tab when double clicking on subgraph
  - handle extern nodes from different bagel_control branches -> relative paths
  - history: save entry when node or edge is deleted
  - selcedtedEdges/selecedNodes is not correct (except when using a frame)

# License {#license}

osg_graph_viz is distributed under the
[3-clause BSD license](https://opensource.org/licenses/BSD-3-Clause).
