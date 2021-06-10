/**
 * \file
 * \copyright
 * Copyright (c) 2012-2021, OpenGeoSys Community (http://www.opengeosys.org)
 *            Distributed under a Modified BSD License.
 *              See accompanying file LICENSE.txt or
 *              http://www.opengeosys.org/project/license
 *
 */

#include "SimpleMatrixVectorProvider.h"

#include <cassert>

#include "BaseLib/Error.h"
#include "BaseLib/Logging.h"
#include "MathLib/LinAlg/LinAlg.h"
#include "MathLib/LinAlg/MatrixVectorTraits.h"

namespace LinAlg = MathLib::LinAlg;

namespace NumLib
{
template <bool do_search, typename MatVec, typename... Args>
std::pair<MatVec*, bool> SimpleMatrixVectorProvider::get_(
    std::size_t& id,
    std::map<std::size_t, MatVec*>& unused_map,
    std::map<MatVec*, std::size_t>& used_map,
    Args&&... args)
{
    id = _next_id++;
    auto res =
        used_map.emplace(MathLib::MatrixVectorTraits<MatVec>::newInstance(
                             std::forward<Args>(args)...)
                             .release(),
                         id);
    return {res.first->first, true};
}

template <bool do_search, typename... Args>
std::pair<GlobalMatrix*, bool> SimpleMatrixVectorProvider::getMatrix_(
    std::size_t& id, Args&&... args)
{
    return get_<do_search>(
        id, _unused_matrices, _used_matrices, std::forward<Args>(args)...);
}

GlobalMatrix& SimpleMatrixVectorProvider::getMatrix()
{
    std::size_t id = 0u;
    return *getMatrix_<false>(id).first;
}

GlobalMatrix& SimpleMatrixVectorProvider::getMatrix(std::size_t& id)
{
    return *getMatrix_<true>(id).first;
}

GlobalMatrix& SimpleMatrixVectorProvider::getMatrix(
    MathLib::MatrixSpecifications const& ms)
{
    std::size_t id = 0u;
    return *getMatrix_<false>(id, ms).first;
    // TODO assert that the returned object always is of the right size
}

GlobalMatrix& SimpleMatrixVectorProvider::getMatrix(
    MathLib::MatrixSpecifications const& ms, std::size_t& id)
{
    return *getMatrix_<true>(id, ms).first;
    // TODO assert that the returned object always is of the right size
}

GlobalMatrix& SimpleMatrixVectorProvider::getMatrix(GlobalMatrix const& A)
{
    std::size_t id = 0u;
    auto const& res = getMatrix_<false>(id, A);
    if (!res.second)
    {  // no new object has been created
        LinAlg::copy(A, *res.first);
    }
    return *res.first;
}

GlobalMatrix& SimpleMatrixVectorProvider::getMatrix(GlobalMatrix const& A,
                                                    std::size_t& id)
{
    auto const& res = getMatrix_<true>(id, A);
    if (!res.second)
    {  // no new object has been created
        LinAlg::copy(A, *res.first);
    }
    return *res.first;
}

void SimpleMatrixVectorProvider::releaseMatrix(GlobalMatrix const& A)
{
    auto it = _used_matrices.find(const_cast<GlobalMatrix*>(&A));
    if (it == _used_matrices.end())
    {
        OGS_FATAL(
            "The given matrix has not been found. Cannot release it. "
            "Aborting.");
    }
    else
    {
        delete it->first;
    }
}

template <bool do_search, typename... Args>
std::pair<GlobalVector*, bool> SimpleMatrixVectorProvider::getVector_(
    std::size_t& id, Args&&... args)
{
    return get_<do_search>(
        id, _unused_vectors, _used_vectors, std::forward<Args>(args)...);
}

GlobalVector& SimpleMatrixVectorProvider::getVector(std::size_t& id)
{
    return *getVector_<true>(id).first;
}

GlobalVector& SimpleMatrixVectorProvider::getVector(
    MathLib::MatrixSpecifications const& ms)
{
    std::size_t id = 0u;
    return *getVector_<false>(id, ms).first;
    // TODO assert that the returned object always is of the right size
}

GlobalVector& SimpleMatrixVectorProvider::getVector(
    MathLib::MatrixSpecifications const& ms, std::size_t& id)
{
    return *getVector_<true>(id, ms).first;
    // TODO assert that the returned object always is of the right size
}

GlobalVector& SimpleMatrixVectorProvider::getVector(GlobalVector const& x)
{
    std::size_t id = 0u;
    auto const& res = getVector_<false>(id, x);
    if (!res.second)
    {  // no new object has been created
        LinAlg::copy(x, *res.first);
    }
    return *res.first;
}

GlobalVector& SimpleMatrixVectorProvider::getVector(GlobalVector const& x,
                                                    std::size_t& id)
{
    auto const& res = getVector_<true>(id, x);
    if (!res.second)
    {  // no new object has been created
        LinAlg::copy(x, *res.first);
    }
    return *res.first;
}

void SimpleMatrixVectorProvider::releaseVector(GlobalVector const& x)
{
    auto it = _used_vectors.find(const_cast<GlobalVector*>(&x));
    if (it == _used_vectors.end())
    {
        OGS_FATAL(
            "The given vector has not been found. Cannot release it. "
            "Aborting.");
    }
    else
    {
        delete it->first;
    }
}

SimpleMatrixVectorProvider::~SimpleMatrixVectorProvider()
{
    if (!_used_matrices.empty())
    {
        WARN(
            "There are still {:d} global matrices in use."
            " This might be an indicator of a possible waste of memory.",
            _used_matrices.size());
    }
    if (!_used_vectors.empty())
    {
        WARN(
            "There are still {:d} global vectors in use."
            " This might be an indicator of a possible waste of memory.",
            _used_vectors.size());
    }

    /*
    for (auto& id_ptr : _unused_matrices)
    {
        delete id_ptr.second;
    }

    for (auto& ptr_id : _used_matrices)
    {
        delete ptr_id.first;
    }

    for (auto& id_ptr : _unused_vectors)
    {
        delete id_ptr.second;
    }

    for (auto& ptr_id : _used_vectors)
    {
        delete ptr_id.first;
    }
    */
}

}  // namespace NumLib
