/**
 * \file
 * \copyright
 * Copyright (c) 2012-2022, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 */

#pragma once

#include <vector>

#include "BaseLib/Logging.h"
#include "LocalAssemblerFactory.h"
#include "NumLib/DOF/LocalToGlobalIndexMap.h"

namespace ProcessLib
{
namespace BoundaryConditionAndSourceTerm
{
namespace detail
{
template <int GlobalDim,
          template <typename, typename, int> class LocalAssemblerImplementation,
          typename LocalAssemblerInterface, typename... ExtraCtorArgs>
void createLocalAssemblers(
    NumLib::LocalToGlobalIndexMap const& dof_table,
    const unsigned shapefunction_order,
    std::vector<MeshLib::Element*> const& mesh_elements,
    std::vector<std::unique_ptr<LocalAssemblerInterface>>& local_assemblers,
    ExtraCtorArgs&&... extra_ctor_args)
{
    static_assert(
        GlobalDim == 1 || GlobalDim == 2 || GlobalDim == 3,
        "Meshes with dimension greater than three are not supported.");

    using LocalAssemblerFactory =
        LocalAssemblerFactory<LocalAssemblerInterface,
                              LocalAssemblerImplementation, GlobalDim,
                              ExtraCtorArgs...>;

    DBUG("Create local assemblers.");

    LocalAssemblerFactory factory(dof_table, shapefunction_order);
    local_assemblers.resize(mesh_elements.size());

    DBUG("Calling local assembler builder for all mesh elements.");
    GlobalExecutor::transformDereferenced(
        factory, mesh_elements, local_assemblers,
        std::forward<ExtraCtorArgs>(extra_ctor_args)...);
}
}  // namespace detail
/*! Creates local assemblers for each element of the given \c mesh.
 *
 * \tparam LocalAssemblerImplementation the individual local assembler type
 * \tparam LocalAssemblerInterface the general local assembler interface
 * \tparam ExtraCtorArgs types of additional constructor arguments.
 *         Those arguments will be passed to the constructor of
 *         \c LocalAssemblerImplementation.
 *
 * The first two template parameters cannot be deduced from the arguments.
 * Therefore they always have to be provided manually.
 */
template <template <typename, typename, int> class LocalAssemblerImplementation,
          typename LocalAssemblerInterface, typename... ExtraCtorArgs>
void createLocalAssemblers(
    const unsigned dimension,
    std::vector<MeshLib::Element*> const& mesh_elements,
    NumLib::LocalToGlobalIndexMap const& dof_table,
    const unsigned shapefunction_order,
    std::vector<std::unique_ptr<LocalAssemblerInterface>>& local_assemblers,
    ExtraCtorArgs&&... extra_ctor_args)
{
    DBUG("Create local assemblers.");

    switch (dimension)
    {
        case 1:
            detail::createLocalAssemblers<1, LocalAssemblerImplementation>(
                dof_table, shapefunction_order, mesh_elements, local_assemblers,
                std::forward<ExtraCtorArgs>(extra_ctor_args)...);
            break;
        case 2:
            detail::createLocalAssemblers<2, LocalAssemblerImplementation>(
                dof_table, shapefunction_order, mesh_elements, local_assemblers,
                std::forward<ExtraCtorArgs>(extra_ctor_args)...);
            break;
        case 3:
            detail::createLocalAssemblers<3, LocalAssemblerImplementation>(
                dof_table, shapefunction_order, mesh_elements, local_assemblers,
                std::forward<ExtraCtorArgs>(extra_ctor_args)...);
            break;
        default:
            OGS_FATAL(
                "Meshes with dimension greater than three are not supported.");
    }
}
}  // namespace BoundaryConditionAndSourceTerm
}  // namespace ProcessLib
