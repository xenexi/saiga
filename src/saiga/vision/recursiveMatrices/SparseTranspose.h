﻿/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/vision/recursiveMatrices/SparseHelper.h"
#include "saiga/vision/recursiveMatrices/Transpose.h"

namespace Saiga
{
/**
 * Sparse Matrix Transposition.
 * This is basically a copy and paste from Eigen/src/SparseCore/SparseMatrix.h :: operator=
 *
 * The only difference is that we call transpose recursivly on each element when assigning them.
 *
 * There are also two additional methods that only transpose the structure/values.
 * This is used for optimization problems with well known structures. In these cases
 * the structure can be precomputed.
 *
 */

template <typename G, typename H, int options>
void transpose(const Eigen::SparseMatrix<G, options>& other, Eigen::SparseMatrix<H, options>& dest)
{
    static_assert(options == Eigen::RowMajor, "todo");
    using SparseMatrix = Eigen::SparseMatrix<G, Eigen::RowMajor>;

    using namespace Eigen;
    //        SparseMatrix dest(other.rows(),other.cols());
    //    dest.resize(other.rows(), other.cols());
    dest.resize(other.cols(), other.rows());
    Eigen::Map<typename SparseMatrix::IndexVector>(dest.outerIndexPtr(), dest.outerSize()).setZero();

    // pass 1
    // FIXME the above copy could be merged with that pass
    for (Index j = 0; j < other.outerSize(); ++j)
        for (typename SparseMatrix::InnerIterator it(other, j); it; ++it) ++dest.outerIndexPtr()[it.index()];

    // prefix sum
    Index count = 0;
    typename SparseMatrix::IndexVector positions(dest.outerSize());
    for (Index j = 0; j < dest.outerSize(); ++j)
    {
        auto tmp                = dest.outerIndexPtr()[j];
        dest.outerIndexPtr()[j] = count;
        positions[j]            = count;
        count += tmp;
    }
    dest.outerIndexPtr()[dest.outerSize()] = count;
    // alloc
    //        dest.m_data.resize(count);
    dest.reserve(count);
    cout << "test" << endl;
    // pass 2
    for (Index j = 0; j < other.outerSize(); ++j)
    {
        for (typename SparseMatrix::InnerIterator it(other, j); it; ++it)
        {
            Index pos                  = positions[it.index()]++;
            dest.innerIndexPtr()[pos]  = j;
            dest.valuePtr()[pos].get() = transpose(it.value()).get();
        }
    }
};


template <typename G, typename H, int options>
void transposeStructureOnly(const Eigen::SparseMatrix<G, options>& other, Eigen::SparseMatrix<H, options>& dest)
{
    static_assert(options == Eigen::RowMajor, "todo");
    using SparseMatrix = Eigen::SparseMatrix<G, Eigen::RowMajor>;

    using namespace Eigen;
    //        SparseMatrix dest(other.rows(),other.cols());
    dest.resize(other.cols(), other.rows());
    Eigen::Map<typename SparseMatrix::IndexVector>(dest.outerIndexPtr(), dest.outerSize()).setZero();

    // pass 1
    // FIXME the above copy could be merged with that pass
    for (Index j = 0; j < other.outerSize(); ++j)
        for (typename SparseMatrix::InnerIterator it(other, j); it; ++it) ++dest.outerIndexPtr()[it.index()];

    // prefix sum
    Index count = 0;
    typename SparseMatrix::IndexVector positions(dest.outerSize());
    for (Index j = 0; j < dest.outerSize(); ++j)
    {
        auto tmp                = dest.outerIndexPtr()[j];
        dest.outerIndexPtr()[j] = count;
        positions[j]            = count;
        count += tmp;
    }
    dest.outerIndexPtr()[dest.outerSize()] = count;
    // alloc
    dest.reserve(count);
    // pass 2
    for (Index j = 0; j < other.outerSize(); ++j)
    {
        for (typename SparseMatrix::InnerIterator it(other, j); it; ++it)
        {
            Index pos                 = positions[it.index()]++;
            dest.innerIndexPtr()[pos] = j;
        }
    }
};



template <typename G, typename H, int options>
void transposeValueOnly(const Eigen::SparseMatrix<G, options>& other, Eigen::SparseMatrix<H, options>& dest)
{
    static_assert(options == Eigen::RowMajor, "todo");
    using SparseMatrix = Eigen::SparseMatrix<G, Eigen::RowMajor>;
    using namespace Eigen;

    std::vector<int> positions(dest.outerSize(), 0);


    for (Index j = 0; j < other.outerSize(); ++j)
    {
        for (typename SparseMatrix::InnerIterator it(other, j); it; ++it)
        {
            Index pos                  = dest.outerIndexPtr()[it.index()] + positions[it.index()]++;
            dest.valuePtr()[pos].get() = transpose(it.value()).get();
        }
    }
};



}  // namespace Saiga