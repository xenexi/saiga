/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include <Eigen/Core>
#include <Eigen/Geometry>
#include "saiga/config.h"
#include "sophus/se3.hpp"


namespace Saiga
{
using SE3 = Sophus::SE3d;

using Quat = Eigen::Quaterniond;

using Vec4 = Eigen::Vector4d;
using Vec3 = Eigen::Vector3d;
using Vec2 = Eigen::Vector2d;

using Mat4 = Eigen::Matrix4d;
using Mat3 = Eigen::Matrix3d;


}  // namespace Saiga