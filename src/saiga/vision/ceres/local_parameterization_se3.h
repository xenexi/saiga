#pragma once
#include "saiga/vision/sophus/Sophus.h"

#include <ceres/autodiff_local_parameterization.h>
#include <ceres/local_parameterization.h>

namespace Sophus
{
namespace test
{
// if this is used in ceres autodiff functors LEFT_MULT should be false
class LocalParameterizationSE3 : public ceres::LocalParameterization
{
   public:
    virtual ~LocalParameterizationSE3() {}

    // SE3 plus operation for Ceres
    //
    //  T * exp(x)
    //
    virtual bool Plus(double const* T_raw, double const* delta_raw, double* T_plus_delta_raw) const
    {
        Eigen::Map<SE3d const> const T(T_raw);
        Eigen::Map<Vector6d const> const delta(delta_raw);
        Eigen::Map<SE3d> T_plus_delta(T_plus_delta_raw);
        T_plus_delta = T * SE3d::exp(delta);

        return true;
    }

    // Jacobian of SE3 plus operation for Ceres
    //
    // Dx T * exp(x)  with  x=0
    //
    virtual bool ComputeJacobian(double const* T_raw, double* jacobian_raw) const
    {
        Eigen::Map<SE3d const> T(T_raw);
        Eigen::Map<Eigen::Matrix<double, 7, 6, Eigen::RowMajor>> jacobian(jacobian_raw);

        // Returns derivative of  this * exp(x)  wrt x at x=0.
        jacobian = T.Dx_this_mul_exp_x_at_0();

        return true;
    }

    virtual int GlobalSize() const { return SE3d::num_parameters; }

    virtual int LocalSize() const { return SE3d::DoF; }
};

// use for analytical diff with lie algebra
template <bool LEFT_MULT = true>
class LocalParameterizationSE32 : public ceres::LocalParameterization
{
   public:
    virtual ~LocalParameterizationSE32() {}

    // SE3 plus operation for Ceres
    //
    //  T * exp(x)
    //
    virtual bool Plus(double const* T_raw, double const* delta_raw, double* T_plus_delta_raw) const
    {
        Eigen::Map<SE3d const> const T(T_raw);
        Eigen::Map<Vector6d const> const delta(delta_raw);
        Eigen::Map<SE3d> T_plus_delta(T_plus_delta_raw);

        if (LEFT_MULT)
        {
            T_plus_delta = SE3d::exp(delta) * T;
        }
        else
        {
            T_plus_delta = T * SE3d::exp(delta);
        }

        return true;
    }

    // Jacobian of SE3 plus operation for Ceres
    //
    // Dx T * exp(x)  with  x=0
    //
    virtual bool ComputeJacobian(double const* T_raw, double* jacobian_raw) const
    {
        Eigen::Map<Eigen::Matrix<double, 7, 6, Eigen::RowMajor>> jacobian_r(jacobian_raw);
        jacobian_r.setZero();
        jacobian_r.block<6, 6>(0, 0).setIdentity();
        return true;
    }

    virtual int GlobalSize() const { return SE3d::num_parameters; }
    virtual int LocalSize() const { return SE3d::DoF; }
};



// use for analytical diff with lie algebra
class LocalParameterizationSE3_Autodiff_Kernel
{
   public:
    virtual ~LocalParameterizationSE3_Autodiff_Kernel() {}

    template <typename T>
    bool operator()(const T* _x, const T* _delta, T* _x_plus_delta) const
    {
        using Vec6 = Eigen::Matrix<T, 6, 1>;
        Eigen::Map<Sophus::SE3<T> const> const x(_x);
        Eigen::Map<Vec6 const> const delta(_delta);
        Eigen::Map<Sophus::SE3<T>> x_plus_delta(_x_plus_delta);

        x_plus_delta = Sophus::SE3<T>::exp(delta) * x;
        return true;
    }
};
using LocalParameterizationSE3_Autodiff =
    ceres::AutoDiffLocalParameterization<LocalParameterizationSE3_Autodiff_Kernel, 7, 6>;

}  // namespace test
}  // namespace Sophus
