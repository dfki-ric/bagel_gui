#ifndef BAGEL_GUI_FORCE_LAYOUT_HPP__
#define BAGEL_GUI_FORCE_LAYOUT_HPP__
#include <osg_graph_viz/Node.hpp>

namespace bagel_gui
{
class ForceLayout {

  // references for node access
  std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> > &nodeMap;
  std::map<osg_graph_viz::Node*, unsigned long> &nodeIdMap;
  std::list<osg::ref_ptr<osg_graph_viz::Edge> > &edgeList;

  // collection of forces
  std::map<unsigned long, float> fx, fy;

  // id of a non moving node
  long fixedNodeId;

public:
  ForceLayout(
      std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> > &nodeMap,
      std::map<osg_graph_viz::Node*, unsigned long> &nodeIdMap,
      std::list<osg::ref_ptr<osg_graph_viz::Edge> > &edgeList )
    : nodeMap( nodeMap ), nodeIdMap( nodeIdMap ), edgeList( edgeList ),
    fixedNodeId(-1)
  {}

  void step() {
    // for each of the nodes, collect the forces, based on repulsion of other
    // nodes, and attraction of edges.
    // This works in two stages, one is collecting the forces, the other one
    // is applying the movements

    fx.clear();
    fy.clear();
    fixedNodeId = -1;

    calcNodes();
    calcEdges();

    updatePositions();
  }

  void calcNodes()
  {
    // first look at the boxes
    std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    for(  it = nodeMap.begin(); it != nodeMap.end(); ++it )
    {
      unsigned long id = it->first;
      osg::ref_ptr<osg_graph_viz::Node> node = it->second;

      // make the first node the fixed node
      if( fixedNodeId == -1 )
        fixedNodeId = id;

      std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> >::iterator it2 = it;
      it2++;
      for( ; it2 != nodeMap.end(); ++it2 )
      {
        unsigned long id2 = it2->first;
        osg::ref_ptr<osg_graph_viz::Node> node2 = it2->second;

        if( node->getParentNode() != node2->getParentNode() )
          continue;

        // create forces between the two rectangles, if they overlap
        double r1x1, r1x2, r1y1, r1y2;
        node->getRectangle( &r1x1, &r1x2, &r1y1, &r1y2 );
        double w1 = r1x2 - r1x1;
        double h1 = r1y2 - r1y1;
        double cx1 = r1x1 + .5 * w1;
        double cy1 = r1y1 + .5 * h1;

        double r2x1, r2x2, r2y1, r2y2;
        node2->getRectangle( &r2x1, &r2x2, &r2y1, &r2y2 );
        double w2 = r2x2 - r2x1;
        double h2 = r2y2 - r2y1;
        double cx2 = r2x1 + .5 * w2;
        double cy2 = r2y1 + .5 * h2;

        double dx = cx1 - cx2;
        double dy = cy1 - cy2;

        double dist = sqrt( dx*dx + dy*dy );

        const double box_diagonal = sqrt( (w1+w2)*(w1+w2) + (h1+h2)*(h1+h2) );
        const double wanted_dist = box_diagonal / 2.0 + 20;

        double disp = 0;
        if( dist > 1e-9 )
        {
          disp = ((dist - wanted_dist) * .1) / dist;
          if( disp > 0 )
            disp = 0;
        }

        fx[id] += dx * disp;
        fy[id] += dy * disp;

        fx[id2] += -dx * disp;
        fy[id2] += -dy * disp;
      }
    }
  }

  void calcEdges()
  {
    // create pull forces between edges
    std::list<osg::ref_ptr<osg_graph_viz::Edge> >::iterator it;
    for( it=edgeList.begin(); it!=edgeList.end(); ++it )
    {
      osg::ref_ptr<osg_graph_viz::Edge> &edge = *it;
      osg::Vec3 v = (edge->getStartPosition() - edge->getEndPosition() );
      double dist = v.length();

      double wanted_dist = 20;
      double disp = 0;
      if( dist > 1e-9 )
      {
        disp = ((dist - wanted_dist) * .1) / dist;
        if( disp < 0 )
          disp = 0;
      }

      unsigned long id = nodeIdMap[edge->getStartNode()];
      unsigned long id2 = nodeIdMap[edge->getEndNode()];

      fx[id] += v.x() * disp;
      fy[id] += v.y() * disp;

      fx[id2] += -v.x() * disp;
      fy[id2] += -v.y() * disp;
    }
  }

  void updatePositions()
  {
    // remove forces from the fixed node
    if( fixedNodeId >= 0 )
    {
      fx[fixedNodeId] = 0;
      fy[fixedNodeId] = 0;
    }

    // apply forces
    std::map<unsigned long, osg::ref_ptr<osg_graph_viz::Node> >::iterator it;
    for(it=nodeMap.begin(); it!=nodeMap.end(); ++it)
    {
      unsigned long id = it->first;
      osg::ref_ptr<osg_graph_viz::Node> node = it->second;

      double x, y;
      node->getPosition( &x, &y );

      x -= fx[id];
      y -= fy[id];

      node->setAbsolutePosition( x, y );
    }
  }
};
}

#endif
