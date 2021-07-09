/**
 * \file
 * \author Karsten Rink
 * \date   2012-05-02
 * \brief  Definition of the Node class.
 *
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#pragma once

#include <cstdlib>
#include <limits>
#include <vector>

#include "MathLib/Point3dWithID.h"

namespace ApplicationUtils
{
    class NodeWiseMeshPartitioner;
}

namespace MeshLib {

class Element;

/**
 * A mesh node with coordinates in 3D space.
 */
class Node : public MathLib::Point3dWithID
{
    /* friend classes: */
    friend class Mesh;
    friend class NodePartitionedMesh;
    friend class MeshRevision;
    friend class MeshLayerMapper;
    friend class ApplicationUtils::NodeWiseMeshPartitioner;

public:
    /// Constructor using a coordinate array
    explicit Node(const double coords[3],
                  std::size_t id = std::numeric_limits<std::size_t>::max());

    /// Constructor using a coordinate array
    explicit Node(std::array<double, 3> const& coords,
                  std::size_t id = std::numeric_limits<std::size_t>::max());

    /// Constructor using single coordinates
    Node(double x, double y, double z, std::size_t id = std::numeric_limits<std::size_t>::max());

    /// Copy constructor
    Node(const Node &node);

    /// Get an element the node is part of.
    const Element* getElement(std::size_t idx) const { return _elements[idx]; }

    /// Get all elements the node is part of.
    const std::vector<Element*>& getElements() const { return _elements; }

    /// Get number of elements the node is part of.
    std::size_t getNumberOfElements() const { return _elements.size(); }

protected:
    /// Update coordinates of a node.
    /// This method automatically also updates the areas/volumes of all connected elements.
    void updateCoordinates(double x, double y, double z);

    /**
     * Add an element the node is part of.
     * This method is called by Mesh::addElement(Element*), see friend definition.
     */
    void addElement(Element* elem) { _elements.push_back(elem); }

    /// clear stored elements connecting to this node
    void clearElements() { _elements.clear(); }

    std::vector<Element*> _elements;
}; /* class */

/// Returns true if the given node is a base node of a (first) element, or if it
/// is not connected to any element i.e. an unconnected node.
bool isBaseNode(Node const& node);

}  // namespace MeshLib
