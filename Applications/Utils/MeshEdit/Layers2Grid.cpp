/**
 * \file
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 */

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// ThirdParty
#include <tclap/CmdLine.h>

#include "BaseLib/IO/readStringListFromFile.h"
#include "GeoLib/AABB.h"
#include "InfoLib/GitInfo.h"
#include "MathLib/Point3d.h"
#include "MeshLib/Elements/Element.h"
#include "MeshLib/IO/VtkIO/VtuInterface.h"
#include "MeshLib/IO/readMeshFromFile.h"
#include "MeshLib/Mesh.h"
#include "MeshLib/MeshEditing/ProjectPointOnMesh.h"
#include "MeshLib/MeshEditing/RemoveMeshComponents.h"
#include "MeshLib/MeshGenerators/MeshGenerator.h"
#include "MeshLib/MeshSearch/MeshElementGrid.h"
#include "MeshLib/Node.h"

static std::string mat_name = "MaterialIDs";

// returns the AABB of all mesh nodes of layers read so far
void adjustExtent(std::pair<MathLib::Point3d, MathLib::Point3d>& extent,
                  MeshLib::Mesh const& mesh)
{
    auto const& nodes = mesh.getNodes();
    GeoLib::AABB aabb(nodes.cbegin(), nodes.cend());
    for (std::size_t i = 0; i < 3; ++i)
    {
        extent.first[i] = std::min(extent.first[i], aabb.getMinPoint()[i]);
        extent.second[i] = std::max(extent.second[i], aabb.getMaxPoint()[i]);
    }
}

// creates a voxel grid of the AABB of all layers
MeshLib::Mesh* generateInitialMesh(
    std::pair<MathLib::Point3d, MathLib::Point3d>& extent,
    std::array<double, 3> const& res)
{
    INFO("Creating initial mesh...");
    std::array<double, 3> mesh_extent{{extent.second[0] - extent.first[0],
                                       extent.second[1] - extent.first[1],
                                       extent.second[2] - extent.first[2]}};
    std::array<std::size_t, 3> const n_cells{
        {static_cast<std::size_t>(std::ceil(mesh_extent[0] / res[0])),
         static_cast<std::size_t>(std::ceil(mesh_extent[1] / res[1])),
         static_cast<std::size_t>(std::ceil(mesh_extent[2] / res[2]))}};
    for (std::size_t i = 0; i < 3; ++i)
    {
        double const ext_range = n_cells[i] * res[i];
        double const offset = (ext_range - mesh_extent[i]) / 2.0;
        mesh_extent[i] = ext_range;
        extent.first[i] -= offset;
        extent.second[i] += offset;
    }
    MeshLib::Mesh* mesh = MeshLib::MeshGenerator::generateRegularHexMesh(
        mesh_extent[0], mesh_extent[1], mesh_extent[2], n_cells[0], n_cells[1],
        n_cells[2], extent.first);
    auto mat_id = mesh->getProperties().createNewPropertyVector<int>(
        mat_name, MeshLib::MeshItemType::Cell);
    if (!mat_id)
    {
        delete mesh;
        return nullptr;
    }
    mat_id->insert(mat_id->end(), mesh->getNumberOfElements(), -1);
    return mesh;
}

// sets material IDs for all elements depending on the layers they are located
// between
void setMaterialIDs(MeshLib::Mesh& mesh,
                    std::vector<std::unique_ptr<MeshLib::Mesh>> const& layers,
                    std::array<double, 3> half_cell_size)
{
    INFO("Setting material properties...");
    std::size_t const n_layers = layers.size();
    auto const& elems = mesh.getElements();
    std::size_t const n_elems = mesh.getNumberOfElements();
    auto mat_ids = mesh.getProperties().getPropertyVector<int>(mat_name);
    std::vector<bool> is_set(n_elems, false);
    for (int i = n_layers - 1; i >= 0; --i)
    {
        INFO("-> Layer {:d}", n_layers - i - 1);
        MeshLib::MeshElementGrid const grid(*layers[i]);
        double const max_edge(layers[i]->getMaxEdgeLength());
        for (std::size_t j = 0; j < n_elems; ++j)
        {
            if (is_set[j])
            {
                continue;
            }

            MeshLib::Node const node = MeshLib::getCenterOfGravity(*elems[j]);
            constexpr double max_val = std::numeric_limits<double>::max();
            MathLib::Point3d const min_vol{
                {node[0] - max_edge, node[1] - max_edge, -max_val}};
            MathLib::Point3d const max_vol{
                {node[0] + max_edge, node[1] + max_edge, max_val}};
            auto const& intersection_candidates =
                grid.getElementsInVolume(min_vol, max_vol);
            auto const* proj_elem =
                MeshLib::ProjectPointOnMesh::getProjectedElement(
                    intersection_candidates, node);
            if (proj_elem == nullptr)
            {
                continue;
            }
            if (node[2] >
                MeshLib::ProjectPointOnMesh::getElevation(*proj_elem, node))
            {
                (*mat_ids)[j] = n_layers - i - 1;
            }
            else
            {
                is_set[j] = true;
            }
        }
    }
    // set all elements above uppermost layer back to -1 so they are
    // subsequently cut
    std::replace(mat_ids->begin(), mat_ids->end(),
                 static_cast<int>(n_layers - 1), -1);
}

// Removes all elements from mesh that have not been marked as being located
// between two layers. If all elements remain unmarked, a nullptr is returned.
MeshLib::Mesh* removeUnusedElements(MeshLib::Mesh const& mesh)
{
    std::vector<std::size_t> marked_elems;
    auto const mat_ids = *MeshLib::materialIDs(mesh);
    std::size_t const n_elems = mat_ids.size();
    for (std::size_t i = 0; i < n_elems; ++i)
    {
        if (mat_ids[i] == -1)
        {
            marked_elems.push_back(i);
        }
    }

    if (marked_elems.size() == mesh.getNumberOfElements())
    {
        return nullptr;
    }
    return MeshLib::removeElements(mesh, marked_elems, "mesh");
}

int main(int argc, char* argv[])
{
    TCLAP::CmdLine cmd(
        "Reads a list of 2D unstructured mesh layers and samples them onto a "
        "structured grid of the same extent. Note, that a large cube size may "
        "result in an undersampling of the original structure.\nCube sizes are "
        "defines by x/y/z-parameters. For equilateral cubes, only the "
        "x-parameter needs to be set.\n\n"
        "OpenGeoSys-6 software, version " +
            GitInfoLib::GitInfo::ogs_version +
            ".\n"
            "Copyright (c) 2012-2021, OpenGeoSys Community "
            "(http://www.opengeosys.org)",
        ' ', GitInfoLib::GitInfo::ogs_version);
    TCLAP::ValueArg<double> z_arg("z", "cellsize-z",
                                  "edge length of cubes in z-direction (depth)",
                                  false, 1000, "floating point number");
    cmd.add(z_arg);

    TCLAP::ValueArg<double> y_arg(
        "y", "cellsize-y", "edge length of cubes in y-direction (latitude)",
        false, 1000, "floating point number");
    cmd.add(y_arg);

    TCLAP::ValueArg<double> x_arg(
        "x", "cellsize-x",
        "edge length of cubes in x-direction (longitude) or all directions, if "
        "y and z are not set",
        true, 1000, "floating point number");
    cmd.add(x_arg);

    TCLAP::ValueArg<std::string> output_arg(
        "o", "output", "name of output mesh (*.vtu)", true, "", "string");
    cmd.add(output_arg);

    TCLAP::ValueArg<std::string> input_arg(
        "i", "input",
        "name of the input file list containing the paths the all input layers "
        "in correct order from top to bottom",
        true, "", "string");
    cmd.add(input_arg);
    cmd.parse(argc, argv);

    if ((y_arg.isSet() && !z_arg.isSet()) ||
        ((!y_arg.isSet() && z_arg.isSet())))
    {
        ERR("For equilateral cubes, only x needs to be set. For unequal "
            "cuboids, all three edge lengths (x/y/z) need to be specified.");
        return EXIT_FAILURE;
    }

    double const x_size = x_arg.getValue();
    double const y_size = (y_arg.isSet()) ? y_arg.getValue() : x_arg.getValue();
    double const z_size = (z_arg.isSet()) ? z_arg.getValue() : x_arg.getValue();
    std::array<double, 3> const cellsize = {x_size, y_size, z_size};

    std::string const input_name = input_arg.getValue();
    std::string const output_name = output_arg.getValue();
    auto const layer_names = BaseLib::IO::readStringListFromFile(input_name);
    if (layer_names.size() < 2)
    {
        ERR("At least two layers are required to create a 3D Mesh");
        return EXIT_FAILURE;
    }

    std::vector<std::unique_ptr<MeshLib::Mesh>> layers;
    layers.reserve(layer_names.size());
    constexpr double minval = std::numeric_limits<double>::max();
    constexpr double maxval = std::numeric_limits<double>::lowest();
    std::pair<MathLib::Point3d, MathLib::Point3d> extent(
        MathLib::Point3d{{minval, minval, minval}},
        MathLib::Point3d{{maxval, maxval, maxval}});
    for (auto const& layer : layer_names)
    {
        std::unique_ptr<MeshLib::Mesh> mesh(
            MeshLib::IO::readMeshFromFile(layer));
        adjustExtent(extent, *mesh);
        layers.emplace_back(std::move(mesh));
    }

    std::unique_ptr<MeshLib::Mesh> mesh(generateInitialMesh(extent, cellsize));
    if (mesh == nullptr)
    {
        ERR("Error creating mesh...");
        return EXIT_FAILURE;
    }
    std::array<double, 3> const half_cell_size{
        {cellsize[0] / 2.0, cellsize[1] / 2.0, cellsize[2] / 2.0}};
    setMaterialIDs(*mesh, layers, half_cell_size);

    std::unique_ptr<MeshLib::Mesh> new_mesh(removeUnusedElements(*mesh));
    if (new_mesh == nullptr)
    {
        ERR("Error generating mesh...");
        return EXIT_FAILURE;
    }

    MeshLib::IO::VtuInterface vtu(new_mesh.get());
    vtu.writeToFile(output_name);
    return EXIT_SUCCESS;
}
