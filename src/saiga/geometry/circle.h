/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/config.h"
#include "saiga/util/math.h"

#include "plane.h"

namespace Saiga
{
/**
 * 3D Circle
 */
class SAIGA_GLOBAL Circle
{
   public:
    vec3 pos;
    float r;
    vec3 normal;

    Circle(void) {}
    Circle(const vec3& p, float r, const vec3& n) : pos(p), r(r), normal(n) {}

    // this circle lays in a plane
    Plane getPlane() const { return Plane(pos, normal); }

    float distance(const vec3& p) const;
    vec3 closestPointOnCircle(const vec3& p) const;


    SAIGA_GLOBAL friend std::ostream& operator<<(std::ostream& os, const Circle& dt);
};

}  // namespace Saiga
